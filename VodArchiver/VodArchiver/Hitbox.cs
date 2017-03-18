using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver {
	[Serializable]
	public class HitboxMediaProfile {
		public string Url;
		public int Height;
		public int Bitrate;

		public HitboxMediaProfile( JToken jt ) {
			Url = (string)jt["url"];
			Height = (int)jt["height"];
			Bitrate = (int)jt["bitrate"];
		}
	}

	[Serializable]
	public class HitboxVideo {
		public string MediaUserName;
		public int MediaId;
		public string MediaFile;
		public int MediaUserId;
		public HitboxMediaProfile[] MediaProfiles;
		public DateTime MediaDateAdded;
		public string MediaTitle;
		public string MediaDescription;
		public string MediaGame;
		public double MediaDuration;
		public int MediaTypeId;

		public HitboxVideo( JToken video ) {
			MediaUserName = (string)video["media_user_name"];
			MediaId = (int)video["media_id"];
			MediaFile = (string)video["media_file"];
			MediaUserId = (int)video["media_user_id"];
			MediaDateAdded = DateTime.Parse( (string)video["media_date_added"] );
			MediaTitle = (string)video["media_title"];
			MediaDescription = (string)video["media_description"];
			MediaGame = (string)video["category_name"];
			try {
				MediaDuration = (double)video["media_duration"];
			} catch ( Exception ) {
				MediaDuration = 0;
			}
			string profileString = (string)video["media_profiles"];
			JToken profiles = JToken.Parse( profileString );
			MediaProfiles = new HitboxMediaProfile[profiles.Count()];
			for ( int i = 0; i < MediaProfiles.Length; ++i ) {
				MediaProfiles[i] = new HitboxMediaProfile( profiles[i] );
			}
			MediaTypeId = (int)video["media_type_id"];
		}
	}

	public class Hitbox {
		static string apiUrl = "http://api.hitbox.tv/";
		public static async Task<HitboxVideo> RetrieveVideo( string id ) {
			Uri uri = new Uri( apiUrl + "media/video/" + id );

			HttpClient client = new HttpClient();
			HttpResponseMessage response = await client.GetAsync( uri );
			if ( response.StatusCode == HttpStatusCode.OK ) {
				string responseString = await response.Content.ReadAsStringAsync();
				return new HitboxVideo( JObject.Parse( responseString )["video"][0] );
			} else {
				throw new Exception( "Hitbox request failed." );
			}
		}

		public static async Task<List<HitboxVideo>> RetrieveVideos( string username, string filter = "recent", int offset = 0, int limit = 100 ) {
			Uri uri = new Uri( apiUrl + "media/video/" + username + "/list?filter=" + filter + "&limit=" + limit + "&offset=" + offset );

			HttpClient client = new HttpClient();
			HttpResponseMessage response = await client.GetAsync( uri );
			if ( response.StatusCode == HttpStatusCode.OK ) {
				string responseString = await response.Content.ReadAsStringAsync();
				JObject jo = JObject.Parse( responseString );
				var jsonVideos = jo["video"];

				List<HitboxVideo> videos = new List<HitboxVideo>();
				foreach ( var v in jsonVideos ) {
					videos.Add( new HitboxVideo( v ) );
				}
				return videos;
			} else {
				throw new Exception( "Hitbox request failed." );
			}
		}
	}
}
