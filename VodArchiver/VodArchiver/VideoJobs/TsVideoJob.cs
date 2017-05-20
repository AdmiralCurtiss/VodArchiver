using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using VodArchiver.VideoInfo;

namespace VodArchiver.VideoJobs {
	[Serializable]
	public abstract class TsVideoJob : IVideoJob {
		public override async Task<ResultType> Run() {
			Stopped = false;
			JobStatus = VideoJobStatus.Running;
			Status = "Retrieving video info...";
			string[] urls = await GetFileUrlsOfVod();
			string combinedFilename = Path.Combine( GetTempFolder(), GetTargetFilenameWithoutExtension() + "_combined.ts" );
			string remuxedTempname = Path.Combine( GetTempFolder(), GetTargetFilenameWithoutExtension() + "_combined.mp4" );
			string remuxedFilename = Path.Combine( GetTargetFolder(), GetTargetFilenameWithoutExtension() + ".mp4" );

			if ( !await Util.FileExists( remuxedFilename ) ) {
				if ( !await Util.FileExists( combinedFilename ) ) {
					Status = "Downloading files...";
					string[] files;
					while ( true ) {
						System.Diagnostics.Stopwatch timer = new System.Diagnostics.Stopwatch();
						timer.Start();
						var downloadResult = await Download( this, GetTempFolderForParts(), urls );
						if ( downloadResult.Item1 != ResultType.Success ) {
							return downloadResult.Item1;
						}
						files = downloadResult.Item2;
						if ( this.VideoInfo.VideoRecordingState != RecordingState.Live ) {
							break;
						} else {
							// we're downloading a stream that is still streaming
							timer.Stop();
							// if too little time has passed wait a bit to allow the stream to provide new data
							if ( timer.Elapsed.TotalMinutes < 2.5 ) {
								TimeSpan ts = TimeSpan.FromMinutes( 2.5 ) - timer.Elapsed;
								Status = "Waiting " + ts.TotalSeconds + " seconds for stream to update...";
								await Task.Delay( ts );
							}
							urls = await GetFileUrlsOfVod();
						}
					}

					Status = "Waiting for free disk IO slot to combine...";
					await Util.ExpensiveDiskIOSemaphore.WaitAsync();
					try {
						Status = "Combining downloaded video parts...";
						await TsVideoJob.Combine( combinedFilename, files );
						await Util.DeleteFiles( files );
						System.IO.Directory.Delete( GetTempFolderForParts() );
					} finally {
						Util.ExpensiveDiskIOSemaphore.Release();
					}
				}

				Status = "Waiting for free disk IO slot to remux...";
				await Util.ExpensiveDiskIOSemaphore.WaitAsync();
				try {
					Status = "Remuxing to MP4...";
					await Task.Run( () => TsVideoJob.Remux( remuxedFilename, combinedFilename, remuxedTempname ) );
				} finally {
					Util.ExpensiveDiskIOSemaphore.Release();
				}
			}

			Status = "Done!";
			JobStatus = VideoJobStatus.Finished;
			return ResultType.Success;
		}

		public abstract Task<string[]> GetFileUrlsOfVod();

		public virtual string GetTempFolder() {
			return Util.TempFolderPath;
		}

		public virtual string GetTempFolderForParts() {
			return System.IO.Path.Combine( Util.TempFolderPath, GetTargetFilenameWithoutExtension() );
		}

		public virtual string GetTargetFolder() {
			return Util.TargetFolderPath;
		}

		public abstract string GetTargetFilenameWithoutExtension();

		public static string GetFolder( string m3u8path ) {
			var urlParts = m3u8path.Trim().Split( '/' );
			urlParts[urlParts.Length - 1] = "";
			return String.Join( "/", urlParts );
		}

		public static string[] GetFilenamesFromM3U8( string m3u8 ) {
			var lines = m3u8.Split( new char[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries );
			List<string> filenames = new List<string>( lines.Length );
			foreach ( var line in lines ) {
				if ( line.Trim() == "" || line.Trim().StartsWith( "#" ) ) {
					continue;
				}

				filenames.Add( line.Trim() );
			}

			return filenames.ToArray();
		}

		public static async Task<(ResultType result, string[] files)> Download( IVideoJob job, string targetFolder, string[] urls, int delayPerDownload = 0 ) {
			Directory.CreateDirectory( targetFolder );

			List<string> files = new List<string>( urls.Length );
			const int MaxTries = 5;
			int triesLeft = MaxTries;
			while ( files.Count < urls.Length ) {
				if ( triesLeft <= 0 ) {
					job.Status = "Failed to download individual parts after " + MaxTries + " tries, aborting.";
					return ( ResultType.Failure, null );
				}
				files.Clear();
				for ( int i = 0; i < urls.Length; ++i ) {
					if ( job.IsStopped() ) {
						job.Status = "Stopped.";
						return ( ResultType.Cancelled, null );
					}

					string url = urls[i];
					string outpath = Path.Combine( targetFolder, "part" + i.ToString( "D8" ) + ".ts" );
					string outpath_temp = outpath + ".tmp";
					if ( await Util.FileExists( outpath_temp ) ) {
						await Util.DeleteFile( outpath_temp );
					}
					if ( await Util.FileExists( outpath ) ) {
						if ( i % 100 == 99 ) {
							job.Status = "Already have part " + ( i + 1 ) + "/" + urls.Length + "...";
						}
						files.Add( outpath );
						continue;
					}

					bool success = false;
					using ( var client = new KeepAliveWebClient() ) {
						try {
							job.Status = "Downloading files... (" + ( files.Count + 1 ) + "/" + urls.Length + ")";
							byte[] data = await client.DownloadDataTaskAsync( url );
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
											throw new VideoDeadException( detail );
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
						}
					}

					if ( success ) {
						File.Move( outpath_temp, outpath );
						files.Add( outpath );
					}

					if ( delayPerDownload > 0 ) {
						await Task.Delay( delayPerDownload );
					}
				}

				if ( files.Count < urls.Length ) {
					await Task.Delay( 60000 );
					--triesLeft;
				}
			}

			return ( ResultType.Success, files.ToArray() );
		}

		public static async Task Combine( string combinedFilename, string[] files ) {
			Console.WriteLine( "Combining into " + combinedFilename + "..." );
			using ( var fs = File.Create( combinedFilename + ".tmp" ) ) {
				foreach ( var file in files ) {
					using ( var part = File.OpenRead( file ) ) {
						await part.CopyToAsync( fs );
						part.Close();
					}
				}

				fs.Close();
			}
			Util.MoveFileOverwrite( combinedFilename + ".tmp", combinedFilename );
		}

		public static void Remux( string targetName, string sourceName, string tempName ) {
			Directory.CreateDirectory( Path.GetDirectoryName( targetName ) );
			Console.WriteLine( "Remuxing to " + targetName + "..." );
			var inputFile = new MediaToolkit.Model.MediaFile( sourceName );
			var outputFile = new MediaToolkit.Model.MediaFile( tempName );
			using ( var engine = new MediaToolkit.Engine( "ffmpeg.exe" ) ) {
				MediaToolkit.Options.ConversionOptions options = new MediaToolkit.Options.ConversionOptions();
				options.AdditionalOptions = "-codec copy -bsf:a aac_adtstoasc";
				engine.Convert( inputFile, outputFile, options );
			}
			Util.MoveFileOverwrite( tempName, targetName );
			Console.WriteLine( "Created " + targetName + "!" );
		}
	}
}
