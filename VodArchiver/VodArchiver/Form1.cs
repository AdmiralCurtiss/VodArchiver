using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using TwixelAPI;

namespace VodArchiver {
	public partial class Form1 : Form {
		Twixel twixel;
		public Form1() {
			InitializeComponent();
			twixel = new Twixel( "", "", Twixel.APIVersion.v3 );
		}

		private async void button1_Click( object sender, EventArgs e ) {
			string id = "v16680710";
			Task<string[]> task = GetFileUrlsOfTwitchVod( twixel, id );
			await task;

			if ( task.Status == TaskStatus.RanToCompletion ) {
				DownloadAndCombineFilePartsTS( id + "-twitch", task.Result );
			} else {
				Console.WriteLine( "error!" );
			}
		}

		public static void DownloadAndCombineFilePartsTS( string targetFolder, string[] urls ) {
			Directory.CreateDirectory( targetFolder );

			// download files
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
							Console.WriteLine( "Downloading " + url + "..." );
							client.DownloadFile( url, outpath_temp );
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

			// combine
			string combinedFilename = Path.Combine( targetFolder, "combined.ts" );
			Console.WriteLine( "Combining into " + combinedFilename + "..." );
			using ( var fs = new FileStream( combinedFilename, FileMode.Create ) ) {
				foreach ( var file in files ) {
					using ( var part = new FileStream( file, FileMode.Open ) ) {
						part.CopyTo( fs );
					}
				}

				fs.Close();
			}

			// remux
			string finalFilename = Path.Combine( targetFolder, "final.mp4" );
			Console.WriteLine( "Remuxing to " + finalFilename + "..." );
			var inputFile = new MediaToolkit.Model.MediaFile( combinedFilename );
			var outputFile = new MediaToolkit.Model.MediaFile( finalFilename );
			using ( var engine = new MediaToolkit.Engine( "ffmpeg.exe" ) ) {
				MediaToolkit.Options.ConversionOptions options = new MediaToolkit.Options.ConversionOptions();
				options.AdditionalOptions = "-codec copy -bsf:a aac_adtstoasc";
				engine.Convert( inputFile, outputFile, options );
			}

			Console.WriteLine( "Created " + finalFilename + "!" );
		}

		public static string GetM3U8PathFromM3U_Twitch( string m3u, string videoType ) {
			var lines = m3u.Split( new char[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries );
			foreach ( var line in lines ) {
				if ( line.Trim() == "" || line.Trim().StartsWith("#") ) {
					continue;
				}

				var urlParts = line.Trim().Split( '/' );
				urlParts[urlParts.Length - 2] = videoType;
				return String.Join( "/", urlParts );
			}

			throw new Exception( m3u + " contains no valid url" );
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

		private static string GetFolder( string m3u8path ) {
			var urlParts = m3u8path.Trim().Split( '/' );
			urlParts[urlParts.Length - 1] = "";
			return String.Join( "/", urlParts );
		}

		public static async Task<string[]> GetFileUrlsOfTwitchVod( Twixel twixel, string vidID ) {
			string m3u = await twixel.RetrieveVodM3U( vidID );
			string m3u8path = GetM3U8PathFromM3U_Twitch( m3u, "chunked" );
			string folderpath = GetFolder( m3u8path );
			string m3u8 = await Twixel.GetWebData( new Uri( m3u8path ) );
			string[] filenames = GetFilenamesFromM3U8( m3u8 );

			List<string> urls = new List<string>( filenames.Length );
			foreach ( var filename in filenames ) {
				urls.Add( folderpath + filename );
			}
			return urls.ToArray();
		}
	}
}
