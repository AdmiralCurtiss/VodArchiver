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

		public string GetTempFilenameWithoutExtension() {
			return "twitch_" + VideoInfo.Username + "_v" + VideoInfo.VideoId + "_" + "chat_gql";
		}

		public string GetTargetFilenameWithoutExtension() {
			return "twitch_" + VideoInfo.Username + "_" + VideoInfo.VideoTimestamp.ToString("yyyy-MM-dd_HH-mm-ss") + "_v" + VideoInfo.VideoId + "_" + "chat_gql";
		}

		public override string GenerateOutputFilename() {
			return GetTargetFilenameWithoutExtension() + ".json";
		}

		public override async Task<ResultType> Run( CancellationToken cancellationToken ) {
			if ( cancellationToken.IsCancellationRequested ) { return ResultType.Cancelled; }

			JobStatus = VideoJobStatus.Running;
			Status = "Retrieving video info...";
			var video_json = await TwitchYTDL.GetVideoJson(long.Parse(VideoInfo.VideoId));
			VideoInfo = new TwitchVideoInfo(TwitchYTDL.VideoFromJson(video_json), StreamService.TwitchChatReplay);
			if ( !AssumeFinished && VideoInfo.VideoRecordingState == RecordingState.Live ) {
				_UserInputRequest = new UserInputRequestStreamLive( this );
				return ResultType.TemporarilyUnavailable;
			}

			string tempfolder = Path.Combine(Util.TempFolderPath, GetTempFilenameWithoutExtension());
			string tempname = Path.Combine(tempfolder, "a.json");
			string filename = Path.Combine(Util.TargetFolderPath, GetTargetFilenameWithoutExtension() + ".json");

			if (Directory.Exists(tempfolder)) {
				Directory.Delete(tempfolder);
			}
			Directory.CreateDirectory(tempfolder);

			Status = "Downloading chat data...";
			var result = await ExternalProgramExecution.RunProgram(@"TwitchDownloaderCLI\TwitchDownloaderCLI.exe", new string[] {
				"chatdownload", "-E", "--stv", "false", "--chat-connections", "1", "--id", VideoInfo.VideoId, "-o", tempname
			});

			Util.MoveFileOverwrite(tempname, filename);

			Directory.Delete(tempfolder);

			Status = "Done!";
			JobStatus = VideoJobStatus.Finished;
			return ResultType.Success;
		}

		protected override bool ShouldStallWrite( string path, long filesize ) {
			long freeSpace = new System.IO.DriveInfo( path ).AvailableFreeSpace;
			return freeSpace <= ( Math.Min( Util.AbsoluteMinimumFreeSpaceBytes, Util.MinimumFreeSpaceBytes ) + filesize );
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
