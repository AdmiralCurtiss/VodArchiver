using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using VodArchiver.VideoInfo;
using System.Xml;
using Newtonsoft.Json.Linq;
using System.Threading;

namespace VodArchiver.VideoJobs {
	class TwitchChatReplayJob : IVideoJob {
		public TwitchChatReplayJob( string id, StatusUpdate.IStatusUpdate statusUpdater = null ) {
			JobStatus = VideoJobStatus.NotStarted;
			StatusUpdater = statusUpdater == null ? new StatusUpdate.NullStatusUpdate() : statusUpdater;
			VideoInfo = new GenericVideoInfo() { Service = StreamService.TwitchChatReplay, VideoId = id };
		}

		public TwitchChatReplayJob( XmlNode node ) : base( node ) { }

		public override bool IsWaitingForUserInput => false;
		public override IUserInputRequest UserInputRequest => _UserInputRequest;
		private IUserInputRequest _UserInputRequest = null;

		public bool AssumeFinished = false;

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", "TwitchChatReplayJob" );
			return base.Serialize( document, node );
		}

		public string GetTargetFilenameWithoutExtension() {
			return "twitch_" + VideoInfo.Username + "_" + VideoInfo.VideoId + "_" + "chat5";
		}

		public override async Task<ResultType> Run( CancellationToken cancellationToken ) {
			if ( cancellationToken.IsCancellationRequested ) { return ResultType.Cancelled; }

			JobStatus = VideoJobStatus.Running;
			Status = "Retrieving video info...";
			VideoInfo = new TwitchVideoInfo( await TwitchV5.GetVideo( long.Parse( VideoInfo.VideoId ) ), StreamService.TwitchChatReplay );
			if ( !AssumeFinished && VideoInfo.VideoRecordingState == RecordingState.Live ) {
				_UserInputRequest = new UserInputRequestStreamLive( this );
				return ResultType.TemporarilyUnavailable;
			}

			string tempname = Path.Combine( Util.TempFolderPath, GetTargetFilenameWithoutExtension() + ".json.tmp" );
			string finalintmpname = Path.Combine( Util.TempFolderPath, GetTargetFilenameWithoutExtension() + ".json" );
			string filename = Path.Combine( Util.TargetFolderPath, GetTargetFilenameWithoutExtension() + ".json" );
			Random rng = new Random( int.Parse( VideoInfo.VideoId.Substring( 1 ) ) );

			if ( !await Util.FileExists( filename ) ) {
				if ( !await Util.FileExists( finalintmpname ) ) {
					Status = "Downloading chat (Initial)...";
					StringBuilder concatJson = new StringBuilder();
					string url = GetStartUrl( VideoInfo );
					int attemptsLeft = 5;
					while ( true ) {
						using ( var client = new KeepAliveWebClient() )
						using ( var cancellationCallback = cancellationToken.Register( client.CancelAsync ) ) {
							try {
								try {
									await Task.Delay( rng.Next( 90000, 270000 ), cancellationToken );
								} catch ( TaskCanceledException ) {
									return ResultType.Cancelled;
								}

								string commentJson = await TwitchV5.Get( url );
								JObject responseObject = JObject.Parse( commentJson );
								if ( responseObject["comments"] == null ) {
									throw new Exception( "Nonsense JSON returned, no comments." );
								}

								string offset = "Unknown";
								try {
									JToken c = ( (JArray)responseObject["comments"] ).Last;
									double val = (double)c["content_offset_seconds"];
									offset = TimeSpan.FromSeconds( val ).ToString();
								} catch ( Exception ) { }

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

					await StallWrite( tempname, concatJson.Length, cancellationToken ); // size not accurate because encoding but whatever
					if ( cancellationToken.IsCancellationRequested ) { return ResultType.Cancelled; }
					File.WriteAllText( tempname, concatJson.ToString() );
					File.Move( tempname, finalintmpname );
				}

				if ( cancellationToken.IsCancellationRequested ) { return ResultType.Cancelled; }

				Status = "Waiting for free disk IO slot to move...";
				try {
					await Util.ExpensiveDiskIOSemaphore.WaitAsync( cancellationToken );
				} catch ( OperationCanceledException ) {
					return ResultType.Cancelled;
				}
				try {
					if ( cancellationToken.IsCancellationRequested ) { return ResultType.Cancelled; }
					Status = "Moving to final location...";
					await StallWrite( filename, new FileInfo( finalintmpname ).Length, cancellationToken );
					if ( cancellationToken.IsCancellationRequested ) { return ResultType.Cancelled; }
					Util.MoveFileOverwrite( finalintmpname, filename );
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

		protected override bool ShouldStallWrite( string path, long filesize ) {
			long freeSpace = new System.IO.DriveInfo( path ).AvailableFreeSpace;
			return freeSpace <= ( Util.AbsoluteMinimumFreeSpaceBytes + filesize ) || freeSpace <= ( Util.MinimumFreeSpaceBytes + filesize );
		}

		class UserInputRequestStreamLive : IUserInputRequest {
			TwitchChatReplayJob Job;

			public UserInputRequestStreamLive( TwitchChatReplayJob job ) {
				Job = job;
			}

			public List<string> GetOptions() {
				return new List<string>() { "Keep Waiting", "Assume Finished" };
			}

			public string GetQuestion() {
				return "Stream still Live";
			}

			public void SelectOption( string option ) {
				Job.AssumeFinished = option == "Assume Finished";
			}
		}
	}
}
