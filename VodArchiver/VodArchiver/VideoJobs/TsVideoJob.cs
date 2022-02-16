using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Xml;
using VodArchiver.VideoInfo;

namespace VodArchiver.VideoJobs {
	public abstract class TsVideoJob : IVideoJob {
		public TsVideoJob() : base() { }
		public TsVideoJob( XmlNode node ) : base( node ) { }

		public override bool IsWaitingForUserInput => _IsWaitingForUserInput;
		public override IUserInputRequest UserInputRequest => _UserInputRequest;
		private bool _IsWaitingForUserInput = false;
		private IUserInputRequest _UserInputRequest = null;

		private bool IgnoreTimeDifferenceCombined = false;
		private bool IgnoreTimeDifferenceRemuxed = false;
		private bool AssumeFinished = false;

		public override async Task<ResultType> Run( CancellationToken cancellationToken ) {
			if ( cancellationToken.IsCancellationRequested ) { return ResultType.Cancelled; }

			JobStatus = VideoJobStatus.Running;
			Status = "Retrieving video info...";
			(ResultType getFileUrlsResult, List<DownloadInfo> downloadInfos) = await GetFileUrlsOfVod( cancellationToken );
			if (getFileUrlsResult == ResultType.UserInputRequired) {
				Status = "Need manual fetch of file URLs.";
				return ResultType.UserInputRequired;
			}
			if ( getFileUrlsResult != ResultType.Success ) {
				Status = "Failed retrieving file URLs.";
				return ResultType.Failure;
			}

			string combinedTempname = Path.Combine( GetTempFolder(), GetTempFilenameWithoutExtension() + "_combined.ts" );
			string combinedFilename = Path.Combine( GetTempFolder(), GetFinalFilenameWithoutExtension() + ".ts" );
			string remuxedTempname = Path.Combine( GetTempFolder(), GetTempFilenameWithoutExtension() + "_combined.mp4" );
			string remuxedFilename = Path.Combine( GetTempFolder(), GetFinalFilenameWithoutExtension() + ".mp4" );
			string targetFilename = Path.Combine( GetTargetFolder(), GetFinalFilenameWithoutExtension() + ".mp4" );
			string baseurlfilepath = Path.Combine(GetTempFolder(), GetTempFilenameWithoutExtension() + "_baseurl.txt");
			string tsnamesfilepath = Path.Combine(GetTempFolder(), GetTempFilenameWithoutExtension() + "_tsnames.txt");

			if ( !await Util.FileExists( targetFilename ) ) {
				if ( !await Util.FileExists( combinedFilename ) ) {
					if ( cancellationToken.IsCancellationRequested ) { return ResultType.Cancelled; }

					Status = "Downloading files...";
					string[] files;
					while ( true ) {
						if ( cancellationToken.IsCancellationRequested ) { return ResultType.Cancelled; }

						System.Diagnostics.Stopwatch timer = new System.Diagnostics.Stopwatch();
						timer.Start();
						var downloadResult = await Download(this, cancellationToken, GetTempFolderForParts(), downloadInfos);
						if ( downloadResult.result != ResultType.Success ) {
							return downloadResult.result;
						}
						files = downloadResult.files;
						if ( this.AssumeFinished || this.VideoInfo.VideoRecordingState != RecordingState.Live ) {
							break;
						} else {
							// we're downloading a stream that is still streaming
							timer.Stop();
							// if too little time has passed wait a bit to allow the stream to provide new data
							if ( timer.Elapsed.TotalMinutes < 2.5 ) {
								TimeSpan ts = TimeSpan.FromMinutes( 2.5 ) - timer.Elapsed;
								Status = "Waiting " + ts.TotalSeconds + " seconds for stream to update...";
								_UserInputRequest = new UserInputRequestStreamLive( this );
								try {
									await Task.Delay( ts, cancellationToken );
								} catch ( TaskCanceledException ) {
									return ResultType.Cancelled;
								}
							}
							if (File.Exists(tsnamesfilepath)) {
								File.Delete(tsnamesfilepath);
							}
							(getFileUrlsResult, downloadInfos) = await GetFileUrlsOfVod( cancellationToken );
							if ( getFileUrlsResult != ResultType.Success ) {
								Status = "Failed retrieving file URLs.";
								return ResultType.Failure;
							}
						}
					}
					_UserInputRequest = null;

					Status = "Waiting for free disk IO slot to combine...";
					try {
						await Util.ExpensiveDiskIOSemaphore.WaitAsync( cancellationToken );
					} catch ( OperationCanceledException ) {
						return ResultType.Cancelled;
					}
					try {
						long expectedTargetFilesize = 0;
						foreach ( var file in files ) {
							expectedTargetFilesize += new FileInfo( file ).Length;
						}

						Status = "Combining downloaded video parts...";
						if ( await Util.FileExists( combinedTempname ) ) {
							await Util.DeleteFile( combinedTempname );
						}
						await StallWrite( combinedFilename, expectedTargetFilesize, cancellationToken );
						if ( cancellationToken.IsCancellationRequested ) { return ResultType.Cancelled; }
						ResultType combineResult = await TsVideoJob.Combine(cancellationToken, combinedTempname, files);
						if (combineResult != ResultType.Success) {
							return combineResult;
						}

						// sanity check
						Status = "Sanity check on combined video...";
						TimeSpan actualVideoLength = ( await FFMpegUtil.Probe( combinedTempname ) ).Duration;
						TimeSpan expectedVideoLength = VideoInfo.VideoLength;
						if ( !IgnoreTimeDifferenceCombined && actualVideoLength.Subtract( expectedVideoLength ).Duration() > TimeSpan.FromSeconds( 5 ) ) {
							// if difference is bigger than 5 seconds something is off, report
							Status = "Large time difference between expected (" + expectedVideoLength.ToString() + ") and combined (" + actualVideoLength.ToString() + "), stopping.";
							_UserInputRequest = new UserInputRequestTimeMismatchCombined( this );
							_IsWaitingForUserInput = true;
							return ResultType.UserInputRequired;
						}

						Util.MoveFileOverwrite( combinedTempname, combinedFilename );
						await Util.DeleteFiles( files );
						System.IO.Directory.Delete( GetTempFolderForParts() );
					} finally {
						Util.ExpensiveDiskIOSemaphore.Release();
					}
				}

				Status = "Waiting for free disk IO slot to remux...";
				try {
					await Util.ExpensiveDiskIOSemaphore.WaitAsync( cancellationToken );
				} catch ( OperationCanceledException ) {
					return ResultType.Cancelled;
				}
				try {
					Status = "Remuxing to MP4...";
					if ( await Util.FileExists( remuxedTempname ) ) {
						await Util.DeleteFile( remuxedTempname );
					}
					await StallWrite( remuxedFilename, new FileInfo( combinedFilename ).Length, cancellationToken );
					if ( cancellationToken.IsCancellationRequested ) { return ResultType.Cancelled; }
					await Task.Run( () => TsVideoJob.Remux( remuxedFilename, combinedFilename, remuxedTempname ) );

					// sanity check
					Status = "Sanity check on remuxed video...";
					TimeSpan actualVideoLength = ( await FFMpegUtil.Probe( remuxedFilename ) ).Duration;
					TimeSpan expectedVideoLength = VideoInfo.VideoLength;
					if ( !IgnoreTimeDifferenceRemuxed && actualVideoLength.Subtract( expectedVideoLength ).Duration() > TimeSpan.FromSeconds( 5 ) ) {
						// if difference is bigger than 5 seconds something is off, report
						Status = "Large time difference between expected (" + expectedVideoLength.ToString() + ") and remuxed (" + actualVideoLength.ToString() + "), stopping.";
						_UserInputRequest = new UserInputRequestTimeMismatchRemuxed( this );
						_IsWaitingForUserInput = true;
						return ResultType.UserInputRequired;
					}

					Util.MoveFileOverwrite( remuxedFilename, targetFilename );
				} finally {
					Util.ExpensiveDiskIOSemaphore.Release();
				}
			}

			Status = "Done!";
			JobStatus = VideoJobStatus.Finished;
			if (File.Exists(tsnamesfilepath)) {
				File.Delete(tsnamesfilepath);
			}
			if (File.Exists(baseurlfilepath)) {
				File.Delete(baseurlfilepath);
			}
			return ResultType.Success;
		}

		public abstract Task<(ResultType result, List<DownloadInfo> downloadInfos)> GetFileUrlsOfVod( CancellationToken cancellationToken );

		public virtual string GetTempFolder() {
			return Util.TempFolderPath;
		}

		public virtual string GetTempFolderForParts() {
			return System.IO.Path.Combine( Util.TempFolderPath, GetTempFilenameWithoutExtension() );
		}

		public virtual string GetTargetFolder() {
			return Util.TargetFolderPath;
		}

		public abstract string GetTempFilenameWithoutExtension();

		public abstract string GetFinalFilenameWithoutExtension();

		public static string GetFolder( string m3u8path ) {
			var urlParts = m3u8path.Trim().Split( '/' );
			urlParts[urlParts.Length - 1] = "";
			return String.Join( "/", urlParts );
		}

		public class DownloadInfo {
			public string Url;
			public string FilesystemId;
			public long? Offset;
			public long? Length;
		}

		public static List<DownloadInfo> GetFilenamesFromM3U8(string baseurl, string m3u8) {
			var lines = m3u8.Split( new char[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries );
			List<DownloadInfo> downloadInfos = new List<DownloadInfo>();

			long? offset = null;
			long? length = null;
			foreach (var line in lines) {
				string l = line.Trim();
				if (l == "") {
					continue;
				}
				if (l.StartsWith("#EXT-X-BYTERANGE:")) {
					string[] split = l.Substring("#EXT-X-BYTERANGE:".Length).Split(new char[] { '@' });
					length = long.Parse(split[0]);
					offset = long.Parse(split[1]);
					continue;
				}
				if (l.StartsWith("#")) {
					continue;
				}

				var di = new DownloadInfo() {
					Url = baseurl + l,
					Length = length,
					Offset = offset,
				};
				di.FilesystemId = GenerateFilesystemName(l, length, offset);
				downloadInfos.Add(di);
				length = null;
				offset = null;
			}
			return downloadInfos;
		}

		private static string GenerateFilesystemName(string l, long? length, long? offset) {
			return Util.FileSystemEscapeName(l) + "_" + (length == null ? "" : length.Value.ToString()) + "_" + (offset == null ? "" : offset.Value.ToString());
		}

		public override string GenerateOutputFilename() {
			return GetFinalFilenameWithoutExtension() + ".mp4";
		}

		public static async Task<(ResultType result, string[] files)> Download(
			IVideoJob job, CancellationToken cancellationToken, string targetFolder, List<DownloadInfo> downloadInfos, int delayPerDownload = 0
		) {
			Directory.CreateDirectory( targetFolder );

			List<string> files = new List<string>(downloadInfos.Count);
			const int MaxTries = 5;
			int triesLeft = MaxTries;
			while (files.Count < downloadInfos.Count) {
				if ( triesLeft <= 0 ) {
					job.Status = "Failed to download individual parts after " + MaxTries + " tries, aborting.";
					return (ResultType.Failure, null);
				}
				files.Clear();
				for (int i = 0; i < downloadInfos.Count; ++i) {
				//for (int i = downloadInfos.Count - 1; i >= 0; --i) {
					if ( cancellationToken.IsCancellationRequested ) {
						return (ResultType.Cancelled, null);
					}

					DownloadInfo downloadInfo = downloadInfos[i];
					string outpath = Path.Combine( targetFolder, downloadInfo.FilesystemId + ".ts" );
					string outpath_temp = outpath + ".tmp";
					if ( await Util.FileExists( outpath_temp ) ) {
						await Util.DeleteFile( outpath_temp );
					}
					if ( await Util.FileExists( outpath ) ) {
						if ( i % 100 == 99 ) {
							job.Status = "Already have part " + ( i + 1 ) + "/" + downloadInfos.Count + "...";
						}
						files.Add( outpath );
						continue;
					}

					bool success = false;
					{
						System.Net.WebClient client = null;
						try {
							if (downloadInfo.Length != null && downloadInfo.Offset != null) {
								client = new KeepAliveWebClientWithRange(downloadInfo.Offset.Value, downloadInfo.Offset.Value + downloadInfo.Length.Value - 1);
							} else {
								client = new KeepAliveWebClient();
							}

							job.Status = "Downloading files... (" + ( files.Count + 1 ) + "/" + downloadInfos.Count + ")";
							byte[] data = await client.DownloadDataTaskAsync(downloadInfo.Url);
							await job.StallWrite( outpath_temp, data.LongLength, cancellationToken );
							if ( cancellationToken.IsCancellationRequested ) { return (ResultType.Cancelled, null); }
							using ( FileStream fs = File.Create( outpath_temp ) ) {
								await fs.WriteAsync( data, 0, data.Length );
							}
							success = true;
						} catch ( System.Net.WebException ex ) {
							System.Net.HttpWebResponse httpWebResponse = ex.Response as System.Net.HttpWebResponse;
							if ( httpWebResponse != null ) {
								switch ( httpWebResponse.StatusCode ) {
									case System.Net.HttpStatusCode.NotFound:
										Newtonsoft.Json.Linq.JObject reply = Newtonsoft.Json.Linq.JObject.Parse( new StreamReader( httpWebResponse.GetResponseStream() ).ReadToEnd() );
										string detail = reply["errors"][0]["detail"].ToObject<string>();
										if ( detail == "No chats for this Video" ) {
											return (ResultType.Dead, null);
										}
										break;
									default:
										Console.WriteLine( "Server returned unhandled error code: " + httpWebResponse.StatusCode );
										break;
								}
							} else {
								Console.WriteLine( ex.ToString() );
							}
							continue;
						} finally {
							if (client != null) {
								client.Dispose();
							}
						}
					}

					if ( success ) {
						await job.StallWrite( outpath, new FileInfo( outpath_temp ).Length, cancellationToken );
						if ( cancellationToken.IsCancellationRequested ) { return (ResultType.Cancelled, null); }
						File.Move( outpath_temp, outpath );
						files.Add( outpath );
					}

					if ( delayPerDownload > 0 ) {
						try {
							await Task.Delay( delayPerDownload, cancellationToken );
						} catch ( TaskCanceledException ) {
							return (ResultType.Cancelled, null);
						}
					}
				}

				if (files.Count < downloadInfos.Count) {
					try {
						await Task.Delay( 60000, cancellationToken );
					} catch ( TaskCanceledException ) {
						return (ResultType.Cancelled, null);
					}
					--triesLeft;
				}
			}

			return (ResultType.Success, files.ToArray());
		}

		public static async Task<ResultType> Combine(CancellationToken cancellationToken, string combinedFilename, string[] files) {
			Console.WriteLine( "Combining into " + combinedFilename + "..." );
			string tempname = combinedFilename + ".tmp";
			using (var fs = File.Create(tempname)) {
				foreach ( var file in files ) {
					if (cancellationToken.IsCancellationRequested) {
						fs.Close();
						File.Delete(tempname);
						return ResultType.Cancelled;
					}
					using ( var part = File.OpenRead( file ) ) {
						await part.CopyToAsync( fs );
						part.Close();
					}
				}

				fs.Close();
			}
			Util.MoveFileOverwrite(tempname, combinedFilename);
			return ResultType.Success;
		}

		public static void Remux( string targetName, string sourceName, string tempName ) {
			Directory.CreateDirectory( Path.GetDirectoryName( targetName ) );
			Console.WriteLine( "Remuxing to " + targetName + "..." );
			VodArchiver.ExternalProgramExecution.RunProgramSynchronous(
				"ffmpeg_remux",
				new string[] {
					"-i", sourceName,
					"-codec", "copy",
					"-bsf:a", "aac_adtstoasc",
					tempName
				}
			);
			Util.MoveFileOverwrite( tempName, targetName );
			Console.WriteLine( "Created " + targetName + "!" );
		}

		class UserInputRequestTimeMismatchCombined : IUserInputRequest {
			TsVideoJob Job;

			public UserInputRequestTimeMismatchCombined( TsVideoJob job ) {
				Job = job;
			}

			public List<string> GetOptions() {
				return new List<string>() { "Try again", "Accept despite the mismatch" };
			}

			public string GetQuestion() {
				return "Handle Combined Mismatch";
			}

			public void SelectOption( string option ) {
				if ( option == "Accept despite the mismatch" ) {
					Job.IgnoreTimeDifferenceCombined = true;
				}
				Job._UserInputRequest = null;
				Job._IsWaitingForUserInput = false;
			}
		}

		class UserInputRequestTimeMismatchRemuxed : IUserInputRequest {
			TsVideoJob Job;

			public UserInputRequestTimeMismatchRemuxed( TsVideoJob job ) {
				Job = job;
			}

			public List<string> GetOptions() {
				return new List<string>() { "Try again", "Accept despite the mismatch" };
			}

			public string GetQuestion() {
				return "Handle Remuxed Mismatch";
			}

			public void SelectOption( string option ) {
				if ( option == "Accept despite the mismatch" ) {
					Job.IgnoreTimeDifferenceRemuxed = true;
				}
				Job._UserInputRequest = null;
				Job._IsWaitingForUserInput = false;
			}
		}

		class UserInputRequestStreamLive : IUserInputRequest {
			TsVideoJob Job;

			public UserInputRequestStreamLive( TsVideoJob job ) {
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
