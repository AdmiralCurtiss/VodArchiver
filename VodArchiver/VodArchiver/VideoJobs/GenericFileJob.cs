﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using VodArchiver.VideoInfo;

namespace VodArchiver.VideoJobs {
	[Serializable]
	class GenericFileJob : IVideoJob {
		public GenericFileJob( string url, StatusUpdate.IStatusUpdate statusUpdater = null ) {
			JobStatus = VideoJobStatus.NotStarted;
			StatusUpdater = statusUpdater == null ? new StatusUpdate.NullStatusUpdate() : statusUpdater;
			VideoInfo = new GenericVideoInfo() { Service = StreamService.RawUrl, VideoId = url };
			Status = "...";
		}

		private static string StringToFilename( string basename, string extension ) {
			return Util.MakeStringFileSystemSafeBaseName( basename ) + "." + extension;
		}

		public string GetTempFilename() {
			return GetTargetFilename() + ".tmp";
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

		public override async Task<ResultType> Run() {
			Stopped = false;
			JobStatus = VideoJobStatus.Running;
			Status = "Downloading...";

			string tempFilename = Path.Combine( Util.TempFolderPath, GetTempFilename() );
			string movedFilename = Path.Combine( Util.TempFolderPath, GetTargetFilename() );
			string targetFilename = Path.Combine( Util.TargetFolderPath, GetTargetFilename() );

			if ( !await Util.FileExists( targetFilename ) ) {
				if ( !await Util.FileExists( movedFilename ) ) {
					if ( await Util.FileExists( tempFilename ) ) {
						await Util.DeleteFile( tempFilename );
					}

					bool success = false;
					using ( var client = new KeepAliveWebClient() ) {
						byte[] data = await client.DownloadDataTaskAsync( VideoInfo.VideoId );
						using ( FileStream fs = File.Create( tempFilename ) ) {
							await fs.WriteAsync( data, 0, data.Length );
							success = true;
						}
					}

					if ( success ) {
						File.Move( tempFilename, movedFilename );
					}
				}

				File.Move( movedFilename, targetFilename );
			}

			Status = "Done!";
			JobStatus = VideoJobStatus.Finished;
			return ResultType.Success;
		}
	}
}
