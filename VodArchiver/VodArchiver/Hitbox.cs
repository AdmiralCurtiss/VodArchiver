using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

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

		public HitboxMediaProfile( XmlNode node ) {
			Url = node.Attributes["url"].Value;
			Height = int.Parse( node.Attributes["height"].Value );
			Bitrate = int.Parse( node.Attributes["bitrate"].Value );
		}

		public XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "url", Url );
			node.AppendAttribute( document, "height", Height.ToString() );
			node.AppendAttribute( document, "bitrate", Bitrate.ToString() );
			return node;
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

		public HitboxVideo( XmlNode node ) {
			MediaUserName = node.Attributes["mediaUserName"].Value;
			MediaId = int.Parse( node.Attributes["mediaId"].Value );
			MediaFile = node.Attributes["mediaFile"].Value;
			MediaUserId = int.Parse( node.Attributes["mediaUserId"].Value );
			List<HitboxMediaProfile> profiles = new List<HitboxMediaProfile>();
			foreach ( XmlNode p in node.SelectNodes( "MediaProfile" ) ) {
				profiles.Add( new HitboxMediaProfile( p ) );
			}
			MediaProfiles = profiles.ToArray();
			MediaDateAdded = DateTime.FromBinary( long.Parse( node.Attributes["mediaDateAdded"].Value ) );
			MediaTitle = node.Attributes["mediaTitle"].Value;
			MediaDescription = node.Attributes["mediaDescription"].Value;
			MediaGame = node.Attributes["mediaGame"].Value;
			MediaDuration = double.Parse( node.Attributes["mediaDuration"].Value );
			MediaTypeId = int.Parse( node.Attributes["mediaTypeId"].Value );
		}

		public XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", "HitboxVideo" );
			node.AppendAttribute( document, "mediaUserName", MediaUserName );
			node.AppendAttribute( document, "mediaId", MediaId.ToString() );
			node.AppendAttribute( document, "mediaFile", MediaFile );
			node.AppendAttribute( document, "mediaUserId", MediaUserId.ToString() );
			foreach ( HitboxMediaProfile profile in MediaProfiles ) {
				node.AppendChild( profile.Serialize( document, document.CreateElement( "MediaProfile" ) ) );
			}
			node.AppendAttribute( document, "mediaDateAdded", MediaDateAdded.ToBinary().ToString() );
			node.AppendAttribute( document, "mediaTitle", MediaTitle );
			node.AppendAttribute( document, "mediaDescription", MediaDescription );
			node.AppendAttribute( document, "mediaGame", MediaGame );
			node.AppendAttribute( document, "mediaDuration", MediaDuration.ToString() );
			node.AppendAttribute( document, "mediaTypeId", MediaTypeId.ToString() );
			return node;
		}
	}

	public class Hitbox {
		static string apiUrl = "http://api.smashcast.tv/";
		public static async Task<(bool success, HitboxVideo video)> RetrieveVideo( string id ) {
			Uri uri = new Uri( apiUrl + "media/video/" + id );

			HttpClient client = new HttpClient();
			HttpResponseMessage response = await client.GetAsync( uri );
			if ( response.StatusCode == HttpStatusCode.OK ) {
				string responseString = await response.Content.ReadAsStringAsync();
				return ( true, new HitboxVideo( JObject.Parse( responseString )["video"][0] ) );
			} else {
				return ( false, null );
			}
		}

		public static async Task<(bool success, List<HitboxVideo> videos)> RetrieveVideos( string username, string filter = "recent", int offset = 0, int limit = 100 ) {
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
				return ( true, videos );
			} else {
				return ( false, null );
			}
		}
	}
}
