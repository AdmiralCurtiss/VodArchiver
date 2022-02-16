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

		public override bool IsWaitingForUserInput => false;

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", "FFMpegReencodeJob" );
			return base.Serialize( document, node );
		}

		public override async Task<ResultType> Run( CancellationToken cancellationToken ) {
			JobStatus = VideoJobStatus.Running;
			Status = "Checking files...";

			if ( cancellationToken.IsCancellationRequested ) { return ResultType.Cancelled; }

			string file = VideoInfo.VideoId;
			string path = Path.GetDirectoryName( file );
			string name = Path.GetFileNameWithoutExtension( file );

			List<string> ffmpegOptions;
			string postfixOld;
			string postfixNew;
			string outputformat;
			if ( VideoInfo is FFMpegReencodeJobVideoInfo ) {
				FFMpegReencodeJobVideoInfo ffvi = VideoInfo as FFMpegReencodeJobVideoInfo;
				ffmpegOptions = ffvi.FFMpegOptions;
				postfixOld = ffvi.PostfixOld;
				postfixNew = ffvi.PostfixNew;
				outputformat = ffvi.OutputFormat;
			} else {
				ffmpegOptions = new List<string>() {
					"-c:v", "libx264",
					"-preset", "slower",
					"-crf", "23",
					"-g", "2000",
					"-c:a", "copy",
					"-max_muxing_queue_size", "100000",
				};
				postfixOld = "_chunked";
				postfixNew = "_x264crf23";
				outputformat = "mp4";
			}
			string ext = "." + outputformat;

			string chunked = postfixOld;
			string postfix = postfixNew;
			string newfile = Path.Combine( path, postfix, name.Substring( 0, name.Length - chunked.Length ) + postfix + ext );
			string newfileinlocal = Path.Combine( path, name.Substring( 0, name.Length - chunked.Length ) + postfix + ext );
			string tempfile = Path.Combine( path, name.Substring( 0, name.Length - chunked.Length ) + postfix + "_TEMP" + ext );
			string chunkeddir = Path.Combine( path, chunked );
			string postfixdir = Path.Combine( path, postfix );
			string oldfileinchunked = Path.Combine( chunkeddir, Path.GetFileName( file ) );

			FFProbeResult probe = null;
			string encodeinput = null;
			if ( await Util.FileExists( file ) ) {
				probe = await FFMpegUtil.Probe( file );
				encodeinput = file;
			} else if ( await Util.FileExists( oldfileinchunked ) ) {
				probe = await FFMpegUtil.Probe( oldfileinchunked );
				encodeinput = oldfileinchunked;
			}

			if ( probe != null ) {
				VideoInfo = new FFMpegReencodeJobVideoInfo(file, probe, ffmpegOptions, postfixOld, postfixNew, outputformat);
			}

			// if the input file doesn't exist we might still be in a state where we can set this to finished if the output file already exists, so continue anyway

			bool newfileexists = await Util.FileExists( newfile );
			bool newfilelocalexists = await Util.FileExists( newfileinlocal );
			if ( !newfileexists && !newfilelocalexists ) {
				if ( encodeinput == null ) {
					// neither input nor output exist, bail
					Status = "Missing!";
					return ResultType.Failure;
				}

				if ( await Util.FileExists( tempfile ) ) {
					await Util.DeleteFile( tempfile );
				}

				if ( cancellationToken.IsCancellationRequested ) { return ResultType.Cancelled; }

				Status = "Encoding " + newfile + "...";
				await StallWrite( newfile, new FileInfo( encodeinput ).Length, cancellationToken );
				if ( cancellationToken.IsCancellationRequested ) { return ResultType.Cancelled; }
				Directory.CreateDirectory( postfixdir );
				FFMpegReencodeJobVideoInfo ffmpegVideoInfo = VideoInfo as FFMpegReencodeJobVideoInfo;
				await Reencode( newfile, encodeinput, tempfile, ffmpegVideoInfo.FFMpegOptions );
			}

			if ( !newfileexists && newfilelocalexists ) {
				Directory.CreateDirectory( postfixdir );
				File.Move( newfileinlocal, newfile );
			}

			if ( await Util.FileExists( file ) && !await Util.FileExists( oldfileinchunked ) ) {
				Directory.CreateDirectory( chunkeddir );
				File.Move( file, oldfileinchunked );
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
				"ffmpeg_encode",
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

		public override string GenerateOutputFilename() {
			return "too lazy to extract this right now, whatever, this job type is not important anyway";
		}
	}
}
