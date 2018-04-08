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

			string file = VideoInfo.VideoId;

			FFProbeResult probe = await FFMpegUtil.Probe( file );
			List<string> ffmpegOptions;
			string postfixOld;
			string postfixNew;
			if ( VideoInfo is FFMpegReencodeJobVideoInfo ) {
				FFMpegReencodeJobVideoInfo ffvi = VideoInfo as FFMpegReencodeJobVideoInfo;
				ffmpegOptions = ffvi.FFMpegOptions;
				postfixOld = ffvi.PostfixOld;
				postfixNew = ffvi.PostfixNew;
			} else {
				ffmpegOptions = new List<string>() {
					"-c:v", "libx264",
					"-preset", "slower",
					"-crf", "23",
					"-g", "2000",
					"-c:a", "copy",
				};
				postfixOld = "_chunked";
				postfixNew = "_x264crf23";
			}

			VideoInfo = new FFMpegReencodeJobVideoInfo( file, probe, ffmpegOptions, postfixOld, postfixNew );

			FFMpegReencodeJobVideoInfo ffmpegVideoInfo = VideoInfo as FFMpegReencodeJobVideoInfo;
			string chunked = ffmpegVideoInfo.PostfixOld;
			string postfix = ffmpegVideoInfo.PostfixNew;
			string path = Path.GetDirectoryName( file );
			string name = Path.GetFileNameWithoutExtension( file );
			string ext = Path.GetExtension( file );
			string postfixdir = Path.Combine( path, postfix );

			string newfile = Path.Combine( path, postfix, name.Substring( 0, name.Length - chunked.Length ) + postfix + ext );
			string tempfile = Path.Combine( path, name.Substring( 0, name.Length - chunked.Length ) + postfix + "_TEMP" + ext );

			if ( !await Util.FileExists( newfile ) ) {
				if ( await Util.FileExists( tempfile ) ) {
					await Util.DeleteFile( tempfile );
				}

				if ( cancellationToken.IsCancellationRequested ) { return ResultType.Cancelled; }

				Status = "Encoding " + newfile + "...";
				Directory.CreateDirectory( postfixdir );
				await Reencode( newfile, file, tempfile, ffmpegVideoInfo.FFMpegOptions );

				string chunkeddir = Path.Combine( path, chunked );
				Directory.CreateDirectory( chunkeddir );
				File.Move( file, Path.Combine( chunkeddir, Path.GetFileName( file ) ) );
			}

			Status = "Done!";
			JobStatus = VideoJobStatus.Finished;
			return ResultType.Success;
		}

		private async Task Reencode( string targetName, string sourceName, string tempName, List<string> options ) {
			List<string> args = new List<string>();
			args.Add( "-i" );
			args.Add( sourceName );
			args.AddRange( options );
			args.Add( tempName );

			await VodArchiver.ExternalProgramExecution.RunProgram(
				"ffmpeg",
				args.ToArray(),
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
