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

	public class TwitchVodFetchResult {
		public List<TwitchVideo> Videos;
		public long TotalVideoCount;
	}

	public static class TwitchYTDL {
		public static async Task<JObject> GetVideoJson(long id) {
			List<string> args = new List<string>() {
				"-J", "https://www.twitch.tv/videos/" + id,
			};
			var data = await ExternalProgramExecution.RunProgram(@"yt-dlp", args.ToArray());
			return JObject.Parse(data.StdOut);
		}

		public static TwitchVideo VideoFromJson(JObject json) {
			TwitchVideo v = new TwitchVideo();
			v.ID = long.Parse(((string)json["id"]).TrimStart('v'));
			v.UserID = 0; // we don't get this from yt-dlp and it's not really important anyway
			v.Username = (string)json["uploader"];
			v.Title = (string)json["title"];
			v.Game = "?";
			v.Description = "?";
			v.CreatedAt = Util.DateTimeFromUnixTime(ulong.Parse((string)json["timestamp"], Util.SerializationFormatProvider));
			v.PublishedAt = v.CreatedAt;
			v.Duration = long.Parse((string)json["duration"]);
			v.ViewCount = long.Parse((string)json["view_count"]);
			bool is_live = (bool)json["is_live"];
			bool was_live = (bool)json["was_live"];
			if (is_live || was_live) {
				v.Type = TwitchVideoType.Archive;
			} else {
				v.Type = TwitchVideoType.Upload;
			}
			v.State = is_live ? RecordingState.Live : RecordingState.Recorded;
			return v;
		}
	}

	public static class TwitchAPI {
		private static readonly string Helix = "https://api.twitch.tv/helix/";
		private static readonly string Api = "https://api.twitch.tv/api/";
		private static readonly string Usher = "http://usher.twitch.tv/";

		public static async Task<string> GetVodM3U(long id, string clientId) {
			string token;
			string sig;
			{
				string result = await GetLegacy(Api + "vods/" + id.ToString() + "/access_token", clientId);
				JObject json = JObject.Parse(result);
				token = (string)json["token"];
				sig = (string)json["sig"];
			}

			return await GetLegacy(Usher + "vod/" + id.ToString() + "?nauthsig=" + sig + "&nauth=" + Uri.EscapeDataString(token), clientId);
		}

		public static async Task<long> GetUserIdFromUsername(string username, string clientId, string clientSecret) {
			string result = await Get(Helix + "users?login=" + Uri.EscapeDataString(username), clientId, clientSecret);
			JObject json = JObject.Parse(result);
			return (long)json["data"][0]["id"];
		}

		public static async Task GetVideosInternal(TwitchVodFetchResult r, long channelId, bool highlights, int offset, int limit, string clientId, string clientSecret, string after) {
			string result = await Get(Helix + "videos?user_id=" + channelId + "&first=" + 100 + "&type=" + (highlights ? "highlight" : "archive") + (after != null ? ("&after=" + after) : ""), clientId, clientSecret);
			JObject json = JObject.Parse(result);

			if (r.Videos == null) {
				r.Videos = new List<TwitchVideo>();
			}
			foreach (JObject jv in (JArray)json["data"]) {
				r.Videos.Add(VideoFromJson(jv));
			}

			try {
				var pagination = (JObject)json["pagination"];
				if (pagination != null) {
					string cursor = (string)pagination["cursor"];
					if (cursor != null && cursor != "") {
						await GetVideosInternal(r, channelId, highlights, offset, limit, clientId, clientSecret, cursor);
						return;
					}
				}
			} catch (Exception) {
				return;
			}
		}

		public static async Task<TwitchVodFetchResult> GetVideos(long channelId, bool highlights, int offset, int limit, string clientId, string clientSecret) {
			TwitchVodFetchResult r = new TwitchVodFetchResult();
			await GetVideosInternal(r, channelId, highlights, offset, limit, clientId, clientSecret, null);
			r.TotalVideoCount = 1; // TODO
			return r;
		}

		public static TwitchVideo VideoFromJson(JObject json) {
			TwitchVideo v = new TwitchVideo();
			v.ID = long.Parse(((string)json["id"]).TrimStart('v'));
			v.UserID = long.Parse((string)json["user_id"]);
			v.Username = (string)json["user_name"];
			v.Title = (string)json["title"];
			v.Game = "?";
			v.Description = (string)json["description"];
			v.CreatedAt = DateTime.Parse((string)json["created_at"], Util.SerializationFormatProvider);
			v.PublishedAt = DateTime.Parse((string)json["published_at"], Util.SerializationFormatProvider);
			v.Duration = Util.ParseTwitchDuration((string)json["duration"]);
			v.ViewCount = long.Parse((string)json["view_count"]);
			switch ((string)json["type"]) {
				case "upload": v.Type = TwitchVideoType.Upload; break;
				case "highlight": v.Type = TwitchVideoType.Highlight; break;
				case "archive": v.Type = TwitchVideoType.Archive; break;
				default: throw new Exception("Unknown twitch broadcast type '" + (string)json["broadcast_type"] + "'");
			}
			v.State = RecordingState.Unknown;
			return v;
		}

		public static async Task<string> GetLegacy(string url, string clientId) {
			HttpClient client = new HttpClient();
			client.DefaultRequestHeaders.Add("Accept", "application/vnd.twitchtv.v5+json");
			client.DefaultRequestHeaders.Add("Client-ID", clientId);
			var result = await client.GetAsync(url);
			if (result.IsSuccessStatusCode) {
				return await result.Content.ReadAsStringAsync();
			} else {
				throw new TwitchHttpException() { StatusCode = result.StatusCode };
			}
		}

		private static async Task<string> Get(string url, string clientId, string clientSecret) {
			string OAuthUrl = "https://id.twitch.tv/oauth2/token";
			HttpClient client2 = new HttpClient();
			HttpResponseMessage token_response = (await client2.PostAsync(OAuthUrl, new FormUrlEncodedContent(new Dictionary<string, string> {
				{ "client_id", clientId },
				{ "client_secret", clientSecret },
				{ "grant_type", "client_credentials" },
			}))).EnsureSuccessStatusCode();
			JObject token_json = JObject.Parse(await token_response.Content.ReadAsStringAsync());
			string token = (string)token_json["access_token"];

			HttpClient client = new HttpClient();
			client.DefaultRequestHeaders.Add("Authorization", "Bearer " + token);
			client.DefaultRequestHeaders.Add("Client-ID", clientId);
			var result = await client.GetAsync(url);
			if (result.IsSuccessStatusCode) {
				return await result.Content.ReadAsStringAsync();
			} else {
				throw new TwitchHttpException() { StatusCode = result.StatusCode };
			}
		}
	}
}
