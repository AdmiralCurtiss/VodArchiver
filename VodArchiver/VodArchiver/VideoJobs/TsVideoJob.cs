﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using VodArchiver.VideoInfo;

namespace VodArchiver.VideoJobs {
	[Serializable]
	public abstract class TsVideoJob : IVideoJob {
		public override async Task Run() {
			JobStatus = VideoJobStatus.Running;
			Status = "Retrieving video info...";
			string[] urls = await GetFileUrlsOfVod();
			string tempFolder = GetTempFolder();
			string combinedFilename = Path.Combine( tempFolder, "combined.ts" );
			string remuxedTempname = Path.Combine( tempFolder, "combined.mp4" );
			string remuxedFilename = Path.Combine( GetTargetFolder(), GetTargetFilenameWithoutExtension() + ".mp4" );

			if ( !await Util.FileExists( remuxedFilename ) ) {
				if ( !await Util.FileExists( combinedFilename ) ) {
					Status = "Downloading files...";
					string[] files = await Download( tempFolder, urls );
					Status = "Combining downloaded video parts...";
					await TsVideoJob.Combine( combinedFilename, files );
					await Util.DeleteFiles( files );
				}
				Status = "Remuxing to MP4...";
				await Task.Run( () => TsVideoJob.Remux( remuxedFilename, combinedFilename, remuxedTempname ) );
			}

			Status = "Done!";
			JobStatus = VideoJobStatus.Finished;
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
				if ( await Util.FileExists( outpath_temp ) ) {
					await Util.DeleteFile( outpath_temp );
				}
				if ( await Util.FileExists( outpath ) ) {
					if ( i % 100 == 99 ) {
						Status = "Already have part " + ( i + 1 ) + "/" + urls.Length + "...";
					}
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
