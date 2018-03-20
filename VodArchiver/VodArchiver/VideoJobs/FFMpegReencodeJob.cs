using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Xml;
using VodArchiver.VideoInfo;

namespace VodArchiver.VideoJobs {
	class FFMpegReencodeJob : IVideoJob {
		public FFMpegReencodeJob( string path, StatusUpdate.IStatusUpdate statusUpdater = null ) {
			JobStatus = VideoJobStatus.NotStarted;
			StatusUpdater = statusUpdater == null ? new StatusUpdate.NullStatusUpdate() : statusUpdater;
			VideoInfo = new GenericVideoInfo() { Service = StreamService.FFMpegJob, VideoId = path };
			Status = "...";
		}

		public FFMpegReencodeJob( XmlNode node ) : base( node ) { }

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", "FFMpegReencodeJob" );
			return base.Serialize( document, node );
		}

		public override async Task<ResultType> Run( CancellationToken cancellationToken ) {
			JobStatus = VideoJobStatus.Running;
			Status = "Checking files...";

			if ( cancellationToken.IsCancellationRequested ) { return ResultType.Cancelled; }

			string chunked = "_chunked";
			string postfix = "_x264crf23";
			string file = VideoInfo.VideoId;
			string path = Path.GetDirectoryName( file );
			string name = Path.GetFileNameWithoutExtension( file );
			string ext = Path.GetExtension( file );
			string postfixdir = Path.Combine( path, postfix );

			VideoInfo = await Probe( file );

			string newfile = Path.Combine( path, postfix, name.Substring( 0, name.Length - chunked.Length ) + postfix + ext );
			string tempfile = Path.Combine( path, name.Substring( 0, name.Length - chunked.Length ) + postfix + "_TEMP" + ext );

			if ( !await Util.FileExists( newfile ) ) {
				if ( await Util.FileExists( tempfile ) ) {
					await Util.DeleteFile( tempfile );
				}

				if ( cancellationToken.IsCancellationRequested ) { return ResultType.Cancelled; }

				Status = "Encoding " + newfile + "...";
				Directory.CreateDirectory( postfixdir );
				await Reencode( newfile, file, tempfile );

				string chunkeddir = Path.Combine( path, chunked );
				Directory.CreateDirectory( chunkeddir );
				File.Move( file, Path.Combine( chunkeddir, Path.GetFileName( file ) ) );
			}

			Status = "Done!";
			JobStatus = VideoJobStatus.Finished;
			return ResultType.Success;
		}

		public static async Task<IVideoInfo> Probe( string filename ) {
			ExternalProgramExecution.RunProgramReturnValue retval = await VodArchiver.ExternalProgramExecution.RunProgram(
				"ffprobe",
				new string[] {
					"-show_format",
					"-print_format", "json",
					filename
				}
			);

			JObject jo = JObject.Parse( retval.StdOut );
			var jsonFormat = jo["format"];

			ulong filesize = (ulong)jsonFormat["size"];
			ulong bitrate = (ulong)jsonFormat["bit_rate"];

			GenericVideoInfo info = new GenericVideoInfo {
				Service = StreamService.FFMpegJob,
				VideoTitle = Path.GetFileName( filename ),
				VideoGame = String.Format( "{0:#,#} MB; {1:#,#} kbps", filesize / 1000000, bitrate / 1000 ),
				VideoTimestamp = File.GetCreationTimeUtc( filename ),
				VideoType = VideoFileType.Unknown,
				VideoRecordingState = RecordingState.Recorded,
				Username = Path.GetFileNameWithoutExtension( filename ),
				VideoId = filename,
				VideoLength = TimeSpan.FromSeconds( (double)jsonFormat["duration"] )
			};

			return info;
		}

		private async Task Reencode( string targetName, string sourceName, string tempName ) {
			await VodArchiver.ExternalProgramExecution.RunProgram(
				"ffmpeg",
				new string[] {
					"-i", sourceName,
					"-c:v", "libx264",
					"-preset", "slower",
					"-crf", "23",
					"-g", "2000",
					"-c:a", "copy",
					tempName
				},
				stderrCallbacks: new System.Diagnostics.DataReceivedEventHandler[1] {
					( sender, received ) => {
						if ( !String.IsNullOrEmpty( received.Data ) ) {
							Status = received.Data;
						}
					}
				}
			);
			File.Move( tempName, targetName );
		}
	}
}
