using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using VodArchiver.VideoInfo;
using TwixelAPI;
using System.Xml;
using Newtonsoft.Json.Linq;
using System.Threading;

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
			return "twitch_" + VideoInfo.Username + "_" + VideoInfo.VideoId + "_" + "chat5";
		}

		public override async Task<ResultType> Run( CancellationToken cancellationToken ) {
			JobStatus = VideoJobStatus.Running;
			Status = "Retrieving video info...";
			VideoInfo = new TwitchVideoInfo( await TwitchAPI.RetrieveVideo( VideoInfo.VideoId ), StreamService.TwitchChatReplay );
			if ( VideoInfo.VideoRecordingState == RecordingState.Live ) {
				return ResultType.TemporarilyUnavailable;
			}

			string tempname = Path.Combine( Util.TempFolderPath, GetTargetFilenameWithoutExtension() + ".json" );
			string filename = Path.Combine( Util.TargetFolderPath, GetTargetFilenameWithoutExtension() + ".json" );
			Random rng = new Random( int.Parse( VideoInfo.VideoId.Substring( 1 ) ) );

			if ( !await Util.FileExists( filename ) ) {
				if ( !await Util.FileExists( tempname ) ) {
					Status = "Downloading chat (Initial)...";
					StringBuilder concatJson = new StringBuilder();
					string url = GetStartUrl( VideoInfo );
					int attemptsLeft = 5;
					while ( true ) {
						using ( var client = new KeepAliveWebClient() ) {
							try {
								await Task.Delay( rng.Next( 90000, 270000 ) );
								string commentJson = await Twixel.GetWebData( new Uri( url ), Twixel.APIVersion.v5 );
								JObject responseObject = JObject.Parse( commentJson );
								if ( responseObject["comments"] == null ) {
									throw new Exception( "Nonsense JSON returned, no comments." );
								}

								string offset = "Unknown";
								try {
									JToken c = ( (JArray)responseObject["comments"] ).Last;
									double val = (double)c["content_offset_seconds"];
									offset = TimeSpan.FromSeconds( val ).ToString();
								} catch (Exception) { }

								concatJson.Append( commentJson );
								if ( responseObject["_next"] != null ) {
									string next = (string)responseObject["_next"];
									attemptsLeft = 5;
									Status = "Downloading chat (Last offset: " + offset + "; next file: " + next + ")...";
									url = GetNextUrl( VideoInfo, next );
								} else {
									// presumably done?
									break;
								}
							} catch ( System.Net.WebException ex ) {
								Console.WriteLine( ex.ToString() );
								--attemptsLeft;
								Status = "Downloading chat (Error; " + attemptsLeft + " attempts left)...";
								if ( attemptsLeft <= 0 ) {
									throw;
								}
								continue;
							}
						}
					}

					File.WriteAllText( tempname, concatJson.ToString() );
				}

				Status = "Waiting for free disk IO slot to move...";
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

		public string GetStartUrl( IVideoInfo info ) {
			return "https://api.twitch.tv/v5/videos/" + info.VideoId.Substring( 1 ) + "/comments?content_offset_seconds=0";
		}

		public string GetNextUrl( IVideoInfo info, string next ) {
			return "https://api.twitch.tv/v5/videos/" + info.VideoId.Substring( 1 ) + "/comments?cursor=" + next;
		}
	}
}
