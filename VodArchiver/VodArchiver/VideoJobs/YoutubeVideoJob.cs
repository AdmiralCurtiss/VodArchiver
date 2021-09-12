using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Xml;
using VodArchiver.VideoInfo;

namespace VodArchiver.VideoJobs {
	public class YoutubeVideoJob : IVideoJob {
		public YoutubeVideoJob( string id, StatusUpdate.IStatusUpdate statusUpdater = null ) {
			JobStatus = VideoJobStatus.NotStarted;
			StatusUpdater = statusUpdater == null ? new StatusUpdate.NullStatusUpdate() : statusUpdater;
			VideoInfo = new GenericVideoInfo() { Service = StreamService.Youtube, VideoId = id };
			Status = "...";
		}

		public YoutubeVideoJob( XmlNode node ) : base( node ) { }

		public override bool IsWaitingForUserInput => false;

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", "YoutubeVideoJob" );
			return base.Serialize( document, node );
		}

		public override async Task<ResultType> Run( CancellationToken cancellationToken ) {
			if ( cancellationToken.IsCancellationRequested ) { return ResultType.Cancelled; }

			JobStatus = VideoJobStatus.Running;
			if ( ( VideoInfo as YoutubeVideoInfo ) == null ) {
				Status = "Retrieving video info...";
				var result = await Youtube.RetrieveVideo( VideoInfo.VideoId, VideoInfo.Username );

				switch ( result.result ) {
					case Youtube.RetrieveVideoResult.Success:
						VideoInfo = result.info;
						break;
					case Youtube.RetrieveVideoResult.ParseFailure:
						// this seems to happen randomly from time to time, just retry later
						return ResultType.TemporarilyUnavailable;
					default:
						return ResultType.Failure;
				}
			}

			string filenameWithoutExtension = "youtube_" + VideoInfo.Username + "_" + VideoInfo.VideoTimestamp.ToString( "yyyy-MM-dd" ) + "_" + VideoInfo.VideoId;
			string filename = filenameWithoutExtension + ".mkv";
			string tempFolder = Path.Combine( Util.TempFolderPath, filenameWithoutExtension );
			string tempFilepath = Path.Combine( tempFolder, filename );

			{
				if ( !await Util.FileExists( tempFilepath ) ) {
					if ( cancellationToken.IsCancellationRequested ) { return ResultType.Cancelled; }

					Directory.CreateDirectory( tempFolder );
					Status = "Running youtube-dl...";
					await StallWrite( tempFilepath, 0, cancellationToken ); // don't know expected filesize, so hope we have a sensible value in minimum free space
					if ( cancellationToken.IsCancellationRequested ) { return ResultType.Cancelled; }
					List<string> args = new List<string>() {
						"-f", "bestvideo+bestaudio/best",
						"-o", tempFilepath,
						"--merge-output-format", "mkv",
						"--no-color",
						"--abort-on-error",
						"--abort-on-unavailable-fragment",
					};
					string limit = Util.YoutubeSpeedLimit;
					if ( limit != "" ) {
						args.Add( "--rate-limit" );
						args.Add( limit );
					}
					args.Add( "https://www.youtube.com/watch?v=" + VideoInfo.VideoId );
					var data = await ExternalProgramExecution.RunProgram(
						@"youtube-dl", args.ToArray(),
						stdoutCallbacks: new System.Diagnostics.DataReceivedEventHandler[1] {
							( sender, received ) => {
								if ( !String.IsNullOrEmpty( received.Data ) ) {
									Status = received.Data;
								}
							}
						}
					);
				}

				string finalFilename = GenerateOutputFilename();
				string finalFilepath = Path.Combine( Util.TargetFolderPath, finalFilename );
				if ( File.Exists( finalFilepath ) ) {
					throw new Exception( "File exists: " + finalFilepath );
				}

				Status = "Waiting for free disk IO slot to move...";
				try {
					await Util.ExpensiveDiskIOSemaphore.WaitAsync( cancellationToken );
				} catch ( OperationCanceledException ) {
					return ResultType.Cancelled;
				}
				try {
					// sanity check
					Status = "Sanity check on downloaded video...";
					TimeSpan actualVideoLength = ( await FFMpegUtil.Probe( tempFilepath ) ).Duration;
					TimeSpan expectedVideoLength = VideoInfo.VideoLength;
					if ( actualVideoLength.Subtract( expectedVideoLength ).Duration() > TimeSpan.FromSeconds( 5 ) ) {
						// if difference is bigger than 5 seconds something is off, report
						Status = "Large time difference between expected (" + expectedVideoLength.ToString() + ") and actual (" + actualVideoLength.ToString() + "), stopping.";
						return ResultType.Failure;
					}

					Status = "Moving...";
					await Task.Run( () => Util.MoveFileOverwrite( tempFilepath, finalFilepath ) );
					await Task.Run( () => Directory.Delete( tempFolder ) );
				} finally {
					Util.ExpensiveDiskIOSemaphore.Release();
				}
			}

			Status = "Done!";
			JobStatus = VideoJobStatus.Finished;
			return ResultType.Success;
		}

		public override string GenerateOutputFilename() {
			return "youtube_" + VideoInfo.Username + "_" + VideoInfo.VideoTimestamp.ToString("yyyy-MM-dd") + "_" + VideoInfo.VideoId + "_" + Util.MakeIntercapsFilename(VideoInfo.VideoTitle).Crop(80) + ".mkv";
		}
	}
}
