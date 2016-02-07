using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
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
			Task<string[]> task = GetFileUrlsOfTwitchVod( twixel, "v16680710" );
			await task;

			if ( task.Status == TaskStatus.RanToCompletion ) {
				foreach ( string s in task.Result ) {
					Console.WriteLine( s );
				}
			} else {
				Console.WriteLine( "error!" );
			}
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
