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
	class GenericFileJob : IVideoJob {
		public GenericFileJob( string url, StatusUpdate.IStatusUpdate statusUpdater = null ) {
			JobStatus = VideoJobStatus.NotStarted;
			StatusUpdater = statusUpdater == null ? new StatusUpdate.NullStatusUpdate() : statusUpdater;
			VideoInfo = new GenericVideoInfo() { Service = StreamService.RawUrl, VideoId = url };
			Status = "...";
		}

		public GenericFileJob( XmlNode node ) : base( node ) { }

		public override bool IsWaitingForUserInput => false;

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", "GenericFileJob" );
			return base.Serialize( document, node );
		}

		private static string StringToFilename( string basename, string extension ) {
			return Util.MakeStringFileSystemSafeBaseName( basename ) + "." + extension;
		}

		static long Counter = 0;

		public string GetTempFoldername() {
			long c = Interlocked.Increment(ref Counter);
			return "rawurl_" + c;
		}

		public string GetTargetFilename() {
			// best guess...
			string extension = Util.MakeStringFileSystemSafeBaseName( VideoInfo.VideoId.Trim().Split( '/' ).LastOrDefault().Split( '?' ).FirstOrDefault() ).Split( '_' ).LastOrDefault();
			if ( String.IsNullOrWhiteSpace( extension ) ) {
				extension = "bin";
			}

			string username = VideoInfo.Username.Trim();
			string title = String.IsNullOrWhiteSpace( VideoInfo.VideoTitle ) ? "file" : VideoInfo.VideoTitle.Trim();
			string datetime = VideoInfo.VideoTimestamp == null || VideoInfo.VideoTimestamp == new DateTime() ? null : VideoInfo.VideoTimestamp.ToString( "yyyy-MM-dd_HH-mm-ss" );
			string prefix = String.IsNullOrWhiteSpace( username ) ? datetime : String.IsNullOrWhiteSpace( datetime ) ? username : username + "_" + datetime;
			string basename = String.IsNullOrWhiteSpace( prefix ) ? title : ( prefix + "_" + title );

			return StringToFilename( basename, extension );
		}

		public override string GenerateOutputFilename() {
			return GetTargetFilename();
		}

		public override async Task<ResultType> Run( CancellationToken cancellationToken ) {
			JobStatus = VideoJobStatus.Running;
			Status = "Downloading...";

			string tempFoldername = Path.Combine(Util.TempFolderPath, GetTempFoldername());
			string urlFilename = "url.txt";
			string urlFilepath = Path.Combine(tempFoldername, urlFilename);
			string downloadFilename = "wip.bin";
			string downloadFilepath = Path.Combine(tempFoldername, downloadFilename);
			string progressFilename = "progress.txt";
			string progressFilepath = Path.Combine(tempFoldername, progressFilename);
			string movedFilename = "done.bin";
			string movedFilepath = Path.Combine(tempFoldername, movedFilename);
			string targetFilename = GetTargetFilename();
			string targetFilepath = Path.Combine(Util.TargetFolderPath, targetFilename);

			if (!await Util.FileExists(targetFilepath)) {
				if (!await Util.FileExists(movedFilepath)) {
					if (Directory.Exists(tempFoldername)) {
						Directory.Delete(tempFoldername, true);
					}
					Directory.CreateDirectory(tempFoldername);

					if (cancellationToken.IsCancellationRequested) { return ResultType.Cancelled; }

					File.WriteAllText(urlFilepath, VideoInfo.VideoId);

					var result = await ExternalProgramExecution.RunProgram(
						"wget",
						new string[] {
							"-i", urlFilename, "-o", progressFilename, "-O", downloadFilename
						},
						workingDir: tempFoldername,
						stdoutCallbacks: new System.Diagnostics.DataReceivedEventHandler[1] {
							(sender, received) => {
								if (!String.IsNullOrEmpty(received.Data)) {
									Status = received.Data;
								}
							}
						}
					);

					if (cancellationToken.IsCancellationRequested) { return ResultType.Cancelled; }
					File.Move(downloadFilepath, movedFilepath);
				}

				if (cancellationToken.IsCancellationRequested) { return ResultType.Cancelled; }
				await StallWrite(targetFilepath, new FileInfo(movedFilepath).Length, cancellationToken);
				if (cancellationToken.IsCancellationRequested) { return ResultType.Cancelled; }
				File.Move(movedFilepath, targetFilepath);
				Directory.Delete(tempFoldername, true);
			}

			Status = "Done!";
			JobStatus = VideoJobStatus.Finished;
			return ResultType.Success;
		}
	}
}
