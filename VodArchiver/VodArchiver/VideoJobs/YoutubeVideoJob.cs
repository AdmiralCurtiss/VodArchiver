using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
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

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", "YoutubeVideoJob" );
			return base.Serialize( document, node );
		}

		public override async Task<ResultType> Run() {
			Stopped = false;
			JobStatus = VideoJobStatus.Running;
			if ( ( VideoInfo as YoutubeVideoInfo ) == null ) {
				Status = "Retrieving video info...";
				var result = await Youtube.RetrieveVideo( VideoInfo.VideoId );

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
			string finalFilepath = Path.Combine( Util.TargetFolderPath, filename );

			if ( !await Util.FileExists( finalFilepath ) ) {
				if ( !await Util.FileExists( tempFilepath ) ) {
					Directory.CreateDirectory( tempFolder );
					Status = "Running youtube-dl...";
					var data = await ExternalProgramExecution.RunProgram(
						@"youtube-dl",
						new string[] {
							"-f", "bestvideo+bestaudio[ext=m4a]/bestvideo+bestaudio/best",
							"-o", tempFilepath,
							"--merge-output-format", "mkv",
							"--no-color",
							"--abort-on-error",
							"--abort-on-unavailable-fragment",
							"https://www.youtube.com/watch?v=" + VideoInfo.VideoId
						},
						stdoutCallbacks: new System.Diagnostics.DataReceivedEventHandler[1] {
							( sender, received ) => {
								if ( !String.IsNullOrEmpty( received.Data ) ) {
									Status = received.Data;
								}
							}
						}
					);

				}

				Status = "Waiting for free disk IO slot to move...";
				await Util.ExpensiveDiskIOSemaphore.WaitAsync();
				try {
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
	}
}
