using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using VodArchiver.VideoInfo;

namespace VodArchiver {
	public enum TwitchVideoType { Upload, Archive, Highlight }

	public class TwitchVideo {
		public long ID;
		public long UserID;
		public string Username;
		public string Title;
		public string Game;
		public string Description;
		public DateTime CreatedAt;
		public DateTime PublishedAt;
		public long Duration;
		public long ViewCount;
		public TwitchVideoType Type;
		public RecordingState State;
	}

	public class TwitchHttpException : Exception {
		public System.Net.HttpStatusCode StatusCode;
	}

	public static class TwitchV5 {
		private static readonly string Kraken = "https://api.twitch.tv/kraken/";
		private static readonly string Api = "https://api.twitch.tv/api/";
		private static readonly string Usher = "http://usher.twitch.tv/";

		public static async Task<TwitchVideo> GetVideo( long id ) {
			string result = await Get( Kraken + "videos/" + id.ToString() );
			JObject json = JObject.Parse( result );

			TwitchVideo v = new TwitchVideo();
			v.ID = long.Parse( ( (string)json["_id"] ).TrimStart( 'v' ) );
			v.UserID = long.Parse( (string)json["channel"]["_id"] );
			v.Username = (string)json["channel"]["display_name"];
			v.Title = (string)json["title"];
			v.Game = (string)json["game"];
			v.Description = (string)json["description"];
			v.CreatedAt = DateTime.Parse( (string)json["created_at"], Util.SerializationFormatProvider );
			v.PublishedAt = DateTime.Parse( (string)json["published_at"], Util.SerializationFormatProvider );
			v.Duration = long.Parse( (string)json["length"] );
			v.ViewCount = long.Parse( (string)json["views"] );
			switch ( (string)json["broadcast_type"] ) {
				case "upload": v.Type = TwitchVideoType.Upload; break;
				case "highlight": v.Type = TwitchVideoType.Highlight; break;
				case "archive": v.Type = TwitchVideoType.Archive; break;
				default: throw new Exception( "Unknown twitch broadcast type '" + (string)json["broadcast_type"] + "'" );
			}
			switch ( (string)json["status"] ) {
				case "recorded": v.State = RecordingState.Recorded; break;
				case "recording": v.State = RecordingState.Live; break;
				default: v.State = RecordingState.Unknown; break;
			}

			return v;
		}

		public static async Task<string> GetVodM3U( long id ) {
			string token;
			string sig;
			{
				string result = await Get( Api + "vods/" + id.ToString() + "/access_token" );
				JObject json = JObject.Parse( result );
				token = (string)json["token"];
				sig = (string)json["sig"];
			}

			return await Get( Usher + "vod/" + id.ToString() + "?nauthsig=" + sig + "&nauth=" + Uri.EscapeDataString( token ) );
		}

		public static async Task<string> Get( string url ) {
			HttpClient client = new HttpClient();
			client.DefaultRequestHeaders.Add( "Accept", "application/vnd.twitchtv.v5+json" );
			client.DefaultRequestHeaders.Add( "Client-ID", Util.TwitchClientId );
			var result = await client.GetAsync( url );
			if ( result.IsSuccessStatusCode ) {
				return await result.Content.ReadAsStringAsync();
			} else {
				throw new TwitchHttpException() { StatusCode = result.StatusCode };
			}
		}
	}
}
