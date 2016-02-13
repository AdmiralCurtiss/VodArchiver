using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver.VideoJobs {
	public class HitboxVideoJob : TsVideoJob {
		public override string ServiceName { get; set; }
		public override string Username { get; set; }
		public override string VideoId { get; set; }
		public override string VideoTitle { get; set; }
		public override string VideoGame { get; set; }
		public override DateTime VideoTimestamp { get; set; }
		public override TimeSpan VideoLength { get; set; }
		private string _Status;
		public override string Status {
			get {
				return _Status;
			}
			set {
				_Status = value;
				StatusUpdater.Update();
			}
		}

		public override StatusUpdate.IStatusUpdate StatusUpdater { get; set; }

		HitboxVideo VideoInfo = null;

		public HitboxVideoJob( string id, StatusUpdate.IStatusUpdate statusUpdater = null ) {
			StatusUpdater = statusUpdater == null ? new StatusUpdate.NullStatusUpdate() : statusUpdater;
			ServiceName = "Hitbox";
			Username = "...";
			VideoId = id;
			Status = "...";
		}

		public override async Task<string[]> GetFileUrlsOfVod() {
			VideoInfo = await Hitbox.RetrieveVideo( VideoId );
			Username = VideoInfo.MediaUserName;
			VideoTitle = VideoInfo.MediaTitle;
			VideoGame = VideoInfo.MediaGame;
			VideoTimestamp = VideoInfo.MediaDateAdded;
			VideoLength = TimeSpan.FromSeconds( VideoInfo.MediaDuration );

			// TODO: Figure out how to determine quality when there are multiple.
			string m3u8path = "http://edge.bf.hitbox.tv/static/videos/vods" + GetM3U8PathFromM3U( VideoInfo.MediaProfiles.First().Url );
			string folderpath = TsVideoJob.GetFolder( m3u8path );
			string m3u8;

			HttpClient client = new HttpClient();
			HttpResponseMessage response = await client.GetAsync( new Uri( m3u8path ) );
			if ( response.StatusCode == System.Net.HttpStatusCode.OK ) {
				m3u8 = await response.Content.ReadAsStringAsync();
			} else {
				throw new Exception( "Hitbox M3U8 request failed." );
			}

			string[] filenames = TsVideoJob.GetFilenamesFromM3U8( m3u8 );
			List<string> urls = new List<string>( filenames.Length );
			foreach ( var filename in filenames ) {
				urls.Add( folderpath + filename );
			}
			return urls.ToArray();
		}

		public static string GetM3U8PathFromM3U( string m3u ) {
			var lines = m3u.Split( new char[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries );
			foreach ( var line in lines ) {
				if ( line.Trim() == "" || line.Trim().StartsWith( "#" ) ) {
					continue;
				}

				return line.Trim();
			}

			throw new Exception( m3u + " contains no valid url" );
		}

		public override string GetTargetFilenameWithoutExtension() {
			return "hitbox_" + VideoInfo.MediaUserName + "_" + VideoId;
		}
	}
}
