using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver {
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

	public class HitboxVideo {
		public string MediaUserName;
		public int MediaId;
		public string MediaFile;
		public int MediaUserId;
		public HitboxMediaProfile[] MediaProfiles;
		public string MediaDateAdded;
		public string MediaTitle;
		public string MediaDescription;
		public double MediaDuration;

		public HitboxVideo( JObject jo ) {
			JToken video = jo["video"][0];
			MediaUserName = (string)video["media_user_name"];
			MediaId = (int)video["media_id"];
			MediaFile = (string)video["media_file"];
			MediaUserId = (int)video["media_user_id"];
			MediaDateAdded = (string)video["media_date_added"];
			MediaTitle = (string)video["media_title"];
			MediaDescription = (string)video["media_description"];
			MediaDuration = (double)video["media_duration"];
			string profileString = (string)video["media_profiles"];
			Console.WriteLine( profileString );
			JToken profiles = JToken.Parse( profileString );
			MediaProfiles = new HitboxMediaProfile[profiles.Count()];
			for ( int i = 0; i < MediaProfiles.Length; ++i ) {
				MediaProfiles[i] = new HitboxMediaProfile( profiles[i] );
			}
		}
	}

	public class Hitbox {
		static string apiUrl = "http://api.hitbox.tv/";
		public static async Task<HitboxVideo> RetrieveVideo( string id ) {
			Uri uri = new Uri( apiUrl + "media/video/" + id );

			HttpClient client = new HttpClient();
			HttpResponseMessage response = await client.GetAsync( uri );
			string responseString;
			if ( response.StatusCode == HttpStatusCode.OK ) {
				responseString = await response.Content.ReadAsStringAsync();
				return new HitboxVideo( JObject.Parse( responseString ) );
			} else {
				throw new Exception( "Hitbox request failed." );
			}
		}
	}
}
