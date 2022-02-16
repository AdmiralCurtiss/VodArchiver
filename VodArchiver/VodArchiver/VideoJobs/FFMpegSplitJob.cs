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
	class FFMpegSplitJob : IVideoJob {
		private string SplitTimes;

		public FFMpegSplitJob( string path, string splitTimes, StatusUpdate.IStatusUpdate statusUpdater = null ) {
			JobStatus = VideoJobStatus.NotStarted;
			StatusUpdater = statusUpdater == null ? new StatusUpdate.NullStatusUpdate() : statusUpdater;
			VideoInfo = new GenericVideoInfo() { Service = StreamService.FFMpegJob, VideoId = path, Username = "_split", VideoTitle = splitTimes };
			Status = "...";
			SplitTimes = splitTimes;
		}

		public FFMpegSplitJob( XmlNode node ) : base( node ) {
			SplitTimes = node.Attributes["splitTimes"].Value;
			if ( VideoInfo is GenericVideoInfo ) {
				VideoInfo.VideoTitle = SplitTimes;
			}
		}

		public override bool IsWaitingForUserInput => false;

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", "FFMpegSplitJob" );
			XmlNode n = base.Serialize( document, node );
			n.AppendAttribute( document, "splitTimes", SplitTimes );
			return n;
		}

		private string GenerateOutputName(string inputName) {
			string dir = System.IO.Path.GetDirectoryName(inputName);
			return System.IO.Path.Combine(dir, GenerateOutputFilename());
		}

		public override string GenerateOutputFilename() {
			string inputName = Path.GetFileName(VideoInfo.VideoId);
			string fn = System.IO.Path.GetFileNameWithoutExtension(inputName);
			string ext = System.IO.Path.GetExtension(inputName);
			var split = fn.Split(new char[] { '_' }, StringSplitOptions.None);
			List<string> output = new List<string>();

			bool added = false;
			foreach (string s in split) {
				ulong val;
				if (!added && s.StartsWith("v") && ulong.TryParse(s.Substring(1), out val)) {
					output.Add(s + "-p%d");
					added = true;
				} else {
					output.Add(s);
				}
			}

			if (!added) {
				output.Add("%d");
			}

			return string.Join("_", output) + ext;
		}

		public override async Task<ResultType> Run( CancellationToken cancellationToken ) {
			JobStatus = VideoJobStatus.Running;
			Status = "Checking files...";

			if ( cancellationToken.IsCancellationRequested ) { return ResultType.Cancelled; }

			string originalpath = Path.GetFullPath( VideoInfo.VideoId );
			string newdirpath = Path.Combine(Path.GetDirectoryName(originalpath), DateTime.UtcNow.ToString("yyyyMMddHHmmssfff"));
			if ( File.Exists( newdirpath ) || Directory.Exists( newdirpath ) ) {
				Status = "File or directory at " + newdirpath + " already exists, cancelling.";
				return ResultType.Failure;
			}
			DirectoryInfo di = Directory.CreateDirectory( newdirpath );
			string inname = Path.Combine( di.FullName, Path.GetFileName( originalpath ) );
			if ( File.Exists( inname ) ) {
				Status = "File at " + inname + " already exists, cancelling.";
				return ResultType.Failure;
			}
			File.Move( originalpath, inname );

			string outname = GenerateOutputName( inname );

			List<string> args = new List<string>();
			args.Add( "-i" );
			args.Add( inname );
			args.Add( "-codec" );
			args.Add( "copy" );
			args.Add( "-map" );
			args.Add( "0" );
			args.Add( "-f" );
			args.Add( "segment" );
			args.Add( "-segment_times" );
			args.Add( SplitTimes );
			args.Add( outname );
			args.Add( "-start_number" );
			args.Add( "1" );

			string exampleOutname = outname.Replace( "%d", "0" );
			bool existedBefore = File.Exists( exampleOutname );

			try {
				await Util.ExpensiveDiskIOSemaphore.WaitAsync();
			} catch ( OperationCanceledException ) {
				Status = "Cancelled";
				return ResultType.Cancelled;
			}
			try {
				Status = "Splitting...";
				await StallWrite( exampleOutname, new FileInfo( inname ).Length, cancellationToken );
				await VodArchiver.ExternalProgramExecution.RunProgram( "ffmpeg_split", args.ToArray() );
			} finally {
				Util.ExpensiveDiskIOSemaphore.Release();
			}


			if ( !existedBefore && File.Exists( exampleOutname ) ) {
				// output generated with indices starting at 0 instead of 1, rename
				List<(string input, string output)> l = new List<(string input, string output)>();
				int digit = 0;
				while ( true ) {
					string input = outname.Replace( "%d", digit.ToString() );
					string output = outname.Replace( "%d", ( digit + 1 ).ToString() );
					if ( File.Exists( input ) ) {
						l.Add( (input, output) );
					} else {
						break;
					}
					++digit;
				}

				l.Reverse();

				foreach ( var (input, output) in l ) {
					if ( !File.Exists( output ) ) {
						File.Move( input, output );
					}
				}
			}

			Status = "Done!";
			JobStatus = VideoJobStatus.Finished;
			return ResultType.Success;
		}
	}
}
