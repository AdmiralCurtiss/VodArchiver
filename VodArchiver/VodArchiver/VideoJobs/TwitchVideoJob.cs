using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using VodArchiver.VideoInfo;
using System.Xml;
using System.Threading;
using System.Diagnostics;
using Newtonsoft.Json.Linq;

namespace VodArchiver.VideoJobs {
	public class TwitchVideoJob : TsVideoJob {
		string VideoQuality = "chunked";

		public TwitchVideoJob( string id, StatusUpdate.IStatusUpdate statusUpdater = null ) {
			JobStatus = VideoJobStatus.NotStarted;
			StatusUpdater = statusUpdater == null ? new StatusUpdate.NullStatusUpdate() : statusUpdater;
			VideoInfo = new GenericVideoInfo() { Service = StreamService.Twitch, VideoId = id };
		}

		public TwitchVideoJob( XmlNode node ) : base( node ) {
			VideoQuality = node.Attributes["videoQuality"].Value;
		}

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", "TwitchVideoJob" );
			XmlNode n = base.Serialize( document, node );
			n.AppendAttribute( document, "videoQuality", VideoQuality );
			return n;
		}

		public override async Task<(ResultType result, List<DownloadInfo> downloadInfos)> GetFileUrlsOfVod( CancellationToken cancellationToken ) {
			var video_json = await TwitchYTDL.GetVideoJson(long.Parse(VideoInfo.VideoId));
			VideoInfo = new TwitchVideoInfo(TwitchYTDL.VideoFromJson(video_json));

			string folderpath;
			List<DownloadInfo> downloadInfos;
			while ( true ) {
				try {
					bool interactive = false;
					if ( interactive ) {
						Status = "";
						string tmp1 = Path.Combine( GetTempFolder(), GetTempFilenameWithoutExtension() + "_baseurl.txt" );
						string tmp2 = Path.Combine( GetTempFolder(), GetTempFilenameWithoutExtension() + "_tsnames.txt" );
						string linesbaseurl = TryGetUserCopyBaseurlM3U(tmp1);
						string linestsnames = TryGetUserCopyTsnamesM3U(tmp2);
						if (linesbaseurl == null) {
							File.WriteAllText(tmp1, "get baseurl for m3u8 from https://www.twitch.tv/videos/" + VideoInfo.VideoId);
						}
						if (linestsnames == null) {
							File.WriteAllText(tmp2, "get actual m3u file from https://www.twitch.tv/videos/" + VideoInfo.VideoId);
						}

						if (linesbaseurl == null || linestsnames == null) {
							await Task.Delay(200);
							Process.Start(tmp1);
							await Task.Delay(200);
							Process.Start(tmp2);
							await Task.Delay(200);
							return (ResultType.UserInputRequired, null);
						}

						folderpath = TsVideoJob.GetFolder(GetM3U8PathFromM3U(linesbaseurl, VideoQuality));
						downloadInfos = TsVideoJob.GetFilenamesFromM3U8(folderpath, linestsnames);
					} else {
						string m3u8path = ExtractM3u8FromJson(video_json);
						folderpath = TsVideoJob.GetFolder(m3u8path);
						var client = new System.Net.Http.HttpClient();
						var result = await client.GetAsync(m3u8path);
						string m3u8 = await result.Content.ReadAsStringAsync();
						downloadInfos = TsVideoJob.GetFilenamesFromM3U8(folderpath, m3u8);
					}
				} catch ( TwitchHttpException e ) {
					if ( e.StatusCode == System.Net.HttpStatusCode.NotFound && VideoInfo.VideoRecordingState == RecordingState.Live ) {
						// this can happen on streams that have just started, in this just wait a bit and retry
						try {
							await Task.Delay( 20000, cancellationToken );
						} catch ( TaskCanceledException ) {
							return (ResultType.Cancelled, null);
						}
						video_json = await TwitchYTDL.GetVideoJson(long.Parse(VideoInfo.VideoId));
						VideoInfo = new TwitchVideoInfo(TwitchYTDL.VideoFromJson(video_json));
						continue;
					} else {
						throw;
					}
				}
				break;
			}

			return (ResultType.Success, downloadInfos);
		}

		private string ExtractM3u8FromJson(JToken json) {
			try {
				string url = json["url"].Value<string>();
				if (url != "") {
					return url;
				}
			} catch (Exception) { }

			foreach (JToken jsonval in json["formats"]) {
				try {
					if (jsonval["format_note"].Value<string>() == "Source") {
						return jsonval["url"].Value<string>();
					}
				} catch (Exception) { }
			}

			throw new Exception("failed to extract twitch url");
		}

		private string TryGetUserCopyBaseurlM3U(string tmp) {
			if (File.Exists(tmp)) {
				var lines = System.IO.File.ReadAllText(tmp);
				if (lines.Contains("vod-secure.twitch.tv") || lines.Contains("cloudfront.net")) {
					return lines;
				}
			}
			return null;
		}

		private string TryGetUserCopyTsnamesM3U(string tmp) {
			if (File.Exists(tmp)) {
				var lines = System.IO.File.ReadAllText(tmp);
				if (lines.Contains(".ts") || lines.Contains("autogen:")) {
					return lines;
				}
			}
			return null;
		}

		public static string GetM3U8PathFromM3U( string m3u, string videoType ) {
			var lines = m3u.Split( new char[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries );
			foreach ( var line in lines ) {
				if ( line.Trim() == "" || line.Trim().StartsWith( "#" ) ) {
					continue;
				}

				var urlParts = line.Trim().Split( '/' );
				urlParts[urlParts.Length - 2] = videoType;
				return String.Join( "/", urlParts );
			}

			throw new Exception( m3u + " contains no valid url" );
		}

		public override string GetTempFilenameWithoutExtension() {
			return "twitch_" + VideoInfo.Username + "_v" + VideoInfo.VideoId + "_" + VideoQuality;
		}

		public override string GetFinalFilenameWithoutExtension() {
			return "twitch_" + VideoInfo.Username + "_" + VideoInfo.VideoTimestamp.ToString("yyyy-MM-dd_HH-mm-ss") + "_v" + VideoInfo.VideoId + "_" + Util.MakeIntercapsFilename( VideoInfo.VideoGame ?? "unknown" ) + "_" + Util.MakeIntercapsFilename( VideoInfo.VideoTitle ).Crop( 80 ) + "_" + VideoQuality;
		}
	}
}
