using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using VodArchiver.VideoInfo;

namespace VodArchiver.VideoJobs {
	[Serializable]
	public class YoutubeVideoJob : IVideoJob {
		public YoutubeVideoJob( string id, StatusUpdate.IStatusUpdate statusUpdater = null ) {
			JobStatus = VideoJobStatus.NotStarted;
			StatusUpdater = statusUpdater == null ? new StatusUpdate.NullStatusUpdate() : statusUpdater;
			VideoInfo = new GenericVideoInfo() { Service = StreamService.Youtube, VideoId = id };
			Status = "...";
		}

		public override async Task Run() {
			JobStatus = VideoJobStatus.Running;
			if ( ( VideoInfo as YoutubeVideoInfo ) == null ) {
				Status = "Retrieving video info...";
				VideoInfo = Youtube.RetrieveVideo( VideoInfo.VideoId );
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
					var data = await Util.RunProgram( @"youtube-dl", "-f \"bestvideo+bestaudio[ext=m4a]/bestvideo+bestaudio\" -o \"" + tempFilepath + "\" --merge-output-format mkv --no-color --abort-on-error --abort-on-unavailable-fragment \"https://www.youtube.com/watch?v=" + VideoInfo.VideoId + "\"" );
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
		}
	}
}
