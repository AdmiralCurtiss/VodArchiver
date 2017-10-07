using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using VodArchiver.VideoInfo;
using TwixelAPI;
using System.Xml;

namespace VodArchiver.VideoJobs {
	class TwitchChatReplayJob : IVideoJob {
		public Twixel TwitchAPI;

		public TwitchChatReplayJob( Twixel api, string id, StatusUpdate.IStatusUpdate statusUpdater = null ) {
			JobStatus = VideoJobStatus.NotStarted;
			StatusUpdater = statusUpdater == null ? new StatusUpdate.NullStatusUpdate() : statusUpdater;
			VideoInfo = new GenericVideoInfo() { Service = StreamService.TwitchChatReplay, VideoId = id };
			TwitchAPI = api;
		}

		public TwitchChatReplayJob( XmlNode node ) : base( node ) { }

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", "TwitchChatReplayJob" );
			return base.Serialize( document, node );
		}

		public string GetTargetFilenameWithoutExtension() {
			return "twitch_" + VideoInfo.Username + "_" + VideoInfo.VideoId + "_" + "chat";
		}

		public override async Task<ResultType> Run() {
			Stopped = false;
			JobStatus = VideoJobStatus.Running;
			Status = "Retrieving video info...";
			VideoInfo = new TwitchVideoInfo( await TwitchAPI.RetrieveVideo( VideoInfo.VideoId ), StreamService.TwitchChatReplay );

			string tempname = Path.Combine( Util.TempFolderPath, GetTargetFilenameWithoutExtension() + ".json" );
			string filename = Path.Combine( Util.TargetFolderPath, GetTargetFilenameWithoutExtension() + ".json" );

			if ( !await Util.FileExists( filename ) ) {
				if ( !await Util.FileExists( tempname ) ) {
					Status = "Downloading files...";
					string[] files;
					while ( true ) {
						string[] urls = GetUrls( VideoInfo );
						if ( this.VideoInfo.VideoRecordingState == RecordingState.Live ) {
							urls = urls.Take( Math.Max( urls.Length - 10, 0 ) ).ToArray();
						}
						ResultType result;
						(result, files) = await TsVideoJob.Download( this, System.IO.Path.Combine( Util.TempFolderPath, GetTargetFilenameWithoutExtension() ), urls, 5000 );
						if ( result != ResultType.Success ) {
							return result;
						}
						if ( this.VideoInfo.VideoRecordingState == RecordingState.Live ) {
							await Task.Delay( 90000 );
							VideoInfo = new TwitchVideoInfo( await TwitchAPI.RetrieveVideo( VideoInfo.VideoId ), StreamService.TwitchChatReplay );
						} else {
							break;
						}
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
			return ResultType.Success;
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
