using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver.VideoJobs {
	public abstract class TsVideoJob : IVideoJob {
		public abstract string ServiceName { get; set; }
		public abstract string Username { get; set; }
		public abstract string VideoId { get; set; }
		public abstract string VideoTitle { get; set; }
		public abstract string VideoGame { get; set; }
		public abstract DateTime VideoTimestamp { get; set; }
		public abstract TimeSpan VideoLength { get; set; }
		public abstract string Status { get; set; }

		public abstract StatusUpdate.IStatusUpdate StatusUpdater { get; set; }

		public async Task Run() {
			Status = "Retrieving video info...";
			string[] urls = await GetFileUrlsOfVod();
			string tempFolder = GetTempFolder();
			Status = "Downloading files...";
			string[] files = await Download( tempFolder, urls );
			string combinedFilename = Path.Combine( tempFolder, "combined.ts" );
			Status = "Combining downloaded video parts...";
			await TsVideoJob.Combine( combinedFilename, files );
			string remuxedFilename = Path.Combine( GetTargetFolder(), GetTargetFilenameWithoutExtension() + ".mp4" );
			Status = "Remuxing to MP4...";
			await Task.Run( () => TsVideoJob.Remux( remuxedFilename, combinedFilename ) );
			Status = "Done!";
		}

		public abstract Task<string[]> GetFileUrlsOfVod();

		public virtual string GetTempFolder() {
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

		public async Task<string[]> Download( string targetFolder, string[] urls ) {
			Directory.CreateDirectory( targetFolder );

			List<string> files = new List<string>( urls.Length );
			for ( int i = 0; i < urls.Length; ++i ) {
				string url = urls[i];
				string outpath = Path.Combine( targetFolder, "part" + i.ToString( "D8" ) + ".ts" );
				string outpath_temp = outpath + ".tmp";
				if ( File.Exists( outpath_temp ) ) {
					File.Delete( outpath_temp );
				}
				if ( File.Exists( outpath ) ) {
					Console.WriteLine( "Already have " + url + "..." );
					files.Add( outpath );
					continue;
				}

				using ( var client = new System.Net.WebClient() ) {
					bool success = false;
					while ( !success ) {
						try {
							Status = "Downloading files... (" + ( i + 1 ) + "/" + urls.Length + ")";
							byte[] data = await client.DownloadDataTaskAsync( url );
							using ( FileStream fs = File.Create( outpath_temp ) ) {
								await fs.WriteAsync( data, 0, data.Length );
							}
							success = true;
						} catch ( System.Net.WebException ex ) {
							Console.WriteLine( ex.ToString() );
							continue;
						}
					}
				}

				File.Move( outpath_temp, outpath );
				files.Add( outpath );
			}

			return files.ToArray();
		}

		public static async Task Combine( string combinedFilename, string[] files ) {
			Console.WriteLine( "Combining into " + combinedFilename + "..." );
			using ( var fs = File.Create( combinedFilename ) ) {
				foreach ( var file in files ) {
					using ( var part = File.OpenRead( file ) ) {
						await part.CopyToAsync( fs );
						part.Close();
					}
				}

				fs.Close();
			}
		}

		public static void Remux( string targetName, string sourceName ) {
			Directory.CreateDirectory( Path.GetDirectoryName( targetName ) );
			Console.WriteLine( "Remuxing to " + targetName + "..." );
			var inputFile = new MediaToolkit.Model.MediaFile( sourceName );
			var outputFile = new MediaToolkit.Model.MediaFile( targetName );
			using ( var engine = new MediaToolkit.Engine( "ffmpeg.exe" ) ) {
				MediaToolkit.Options.ConversionOptions options = new MediaToolkit.Options.ConversionOptions();
				options.AdditionalOptions = "-codec copy -bsf:a aac_adtstoasc";
				engine.Convert( inputFile, outputFile, options );
			}
			Console.WriteLine( "Created " + targetName + "!" );
		}
	}
}
