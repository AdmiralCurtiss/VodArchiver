﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using VodArchiver.VideoInfo;
using TwixelAPI;

namespace VodArchiver.VideoJobs {
	[Serializable]
	class TwitchChatReplayJob : IVideoJob {
		[NonSerialized]
		public Twixel TwitchAPI;

		public TwitchChatReplayJob( Twixel api, string id, StatusUpdate.IStatusUpdate statusUpdater = null ) {
			JobStatus = VideoJobStatus.NotStarted;
			StatusUpdater = statusUpdater == null ? new StatusUpdate.NullStatusUpdate() : statusUpdater;
			VideoInfo = new GenericVideoInfo() { Service = StreamService.TwitchChatReplay, VideoId = id };
			TwitchAPI = api;
		}

		public string GetTargetFilenameWithoutExtension() {
			return "twitch_" + VideoInfo.Username + "_" + VideoInfo.VideoId + "_" + "chat";
		}

		public override async Task Run() {
			JobStatus = VideoJobStatus.Running;
			Status = "Retrieving video info...";
			VideoInfo = new TwitchVideoInfo( await TwitchAPI.RetrieveVideo( VideoInfo.VideoId ), StreamService.TwitchChatReplay );

			string tempname = Path.Combine( Util.TempFolderPath, GetTargetFilenameWithoutExtension() + ".json" );
			string filename = Path.Combine( Util.TargetFolderPath, GetTargetFilenameWithoutExtension() + ".json" );

			if ( !await Util.FileExists( filename ) ) {
				if ( !await Util.FileExists( tempname ) ) {
					Status = "Downloading files...";
					string[] files;
					// no clue how a still running stream handles chat so better not mess with that until we know
					if ( this.VideoInfo.VideoRecordingState != RecordingState.Live ) {
						files = await TsVideoJob.Download( this, System.IO.Path.Combine( Util.TempFolderPath, GetTargetFilenameWithoutExtension() ), GetUrls( VideoInfo ), 5000 );
					} else {
						throw new Exception( "Don't know how to deal with running stream's chat." );
					}

					Status = "Waiting for free disk IO slot to combine...";
					await Util.ExpensiveDiskIOSemaphore.WaitAsync();
					try {
						Status = "Combining downloaded chat parts...";
						// this is not a valid way to combine json but whatever, this is trivially fixable later if I ever actually need it
						await TsVideoJob.Combine( tempname, files );
						await Util.DeleteFiles( files );
						System.IO.Directory.Delete( System.IO.Path.Combine( Util.TempFolderPath, GetTargetFilenameWithoutExtension() ) );
					} finally {
						Util.ExpensiveDiskIOSemaphore.Release();
					}
				}

				Status = "Waiting for free disk IO slot to remux...";
				await Util.ExpensiveDiskIOSemaphore.WaitAsync();
				try {
					Status = "Moving to final location...";
					Util.MoveFileOverwrite( tempname, filename );
				} finally {
					Util.ExpensiveDiskIOSemaphore.Release();
				}
			}

			Status = "Done!";
			JobStatus = VideoJobStatus.Finished;
		}

		public string[] GetUrls( IVideoInfo info ) {
			List<string> urls = new List<string>();
			DateTime dt = info.VideoTimestamp;
			while ( dt <= info.VideoTimestamp + info.VideoLength ) {
				string url = @"https://rechat.twitch.tv/rechat-messages?start=" + dt.DateTimeToUnixTime() + "&video_id=" + info.VideoId;
				urls.Add( url );
				dt = dt.AddSeconds( 30.0 );
			}

			return urls.ToArray();
		}
	}
}