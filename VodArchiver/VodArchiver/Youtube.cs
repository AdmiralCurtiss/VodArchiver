using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using VodArchiver.VideoInfo;

namespace VodArchiver {
	public class Youtube {
		public static (bool success, YoutubeVideoInfo info) ParseFromJsonFull( JToken json ) {
			YoutubeVideoInfo y = new YoutubeVideoInfo();
			y.Username = json["uploader_id"].Value<string>();
			if ( string.IsNullOrWhiteSpace( y.Username ) ) {
				return (false, y);
			}
			y.VideoId = json["id"].Value<string>();
			y.VideoTitle = json["title"].Value<string>();

			var tags = json["tags"];
			if ( tags != null ) {
				StringBuilder sb = new StringBuilder();
				foreach ( var tag in tags ) {
					sb.Append( tag.Value<string>() ).Append( "; " );
				}

				if ( sb.Length >= 2 ) {
					sb.Remove( sb.Length - 2, 2 );
					y.VideoGame = sb.ToString();
				} else {
					y.VideoGame = "";
				}
			} else {
				y.VideoGame = "";
			}

			string datetimestring = json["upload_date"].Value<string>();
			if ( string.IsNullOrWhiteSpace( datetimestring ) ) {
				return (false, y);
			}
			y.VideoTimestamp = DateTime.ParseExact( datetimestring, "yyyyMMdd", null );
			y.VideoLength = TimeSpan.FromSeconds( (double)UInt64.Parse( json["duration"].Value<string>() ) );
			y.VideoRecordingState = RecordingState.Recorded;
			y.VideoType = VideoFileType.Unknown;

			y.UserDisplayName = json["uploader"].Value<string>();
			y.VideoDescription = json["description"].Value<string>();
			return (true, y);
		}

		public static (bool success, GenericVideoInfo info) ParseFromJsonFlat( JToken json ) {
			GenericVideoInfo y = new GenericVideoInfo();
			y.Service = StreamService.Youtube;
			y.VideoId = json["id"].Value<string>();
			y.VideoTitle = json["title"].Value<string>();
			return (true, y);
		}

		public static (bool success, IVideoInfo info) ParseFromJson( JToken json, bool flat ) {
			if ( flat ) {
				return ParseFromJsonFlat( json );
			} else {
				return ParseFromJsonFull( json );
			}
		}

		public enum RetrieveVideoResult { Success, FetchFailure, ParseFailure }
		public static async Task<(RetrieveVideoResult result, IVideoInfo info)> RetrieveVideo( string id ) {
			var data = await ExternalProgramExecution.RunProgram( @"youtube-dl", new string[] { "-j", "https://www.youtube.com/watch?v=" + id } );
			var json = JObject.Parse( data.StdOut );
			var parsed = ParseFromJson( json, false );
			return (parsed.success ? RetrieveVideoResult.Success : RetrieveVideoResult.ParseFailure, parsed.info);
		}

		public static async Task<List<IVideoInfo>> RetrieveVideosFromParameterString( string parameter, bool flat ) {
			string raw = "";
			try {
				List<string> args = new List<string>();
				if ( flat ) {
					args.Add( "--flat-playlist" );
				}
				args.Add( "--ignore-errors" );
				args.Add( "-J" );
				args.Add( parameter );
				ExternalProgramExecution.RunProgramReturnValue data = await ExternalProgramExecution.RunProgram( @"youtube-dl", args.ToArray() );
				raw = data.StdOut;
			} catch ( ExternalProgramReturnNonzeroException ex ) {
				// try anyway, this gets thrown when a video is unavailable for copyright reasons
				raw = ex.StdOut;
			}
			var json = JObject.Parse( raw );
			var entries = json["entries"];
			List<IVideoInfo> list = new List<IVideoInfo>();
			foreach ( var entry in entries ) {
				try {
					var d = ParseFromJson( entry, flat );
					if ( d.success ) {
						list.Add( d.info );
					}
				} catch ( Exception ex ) {
					Console.WriteLine( "Failed to parse video from Youtube: " + ex.ToString() );
				}
			}
			return list;
		}

		public static async Task<List<IVideoInfo>> RetrieveVideosFromPlaylist( string playlist, bool flat ) {
			return await RetrieveVideosFromParameterString( "https://www.youtube.com/playlist?list=" + playlist, flat );
		}

		public static async Task<List<IVideoInfo>> RetrieveVideosFromChannel( string channel, bool flat ) {
			return await RetrieveVideosFromParameterString( "https://www.youtube.com/channel/" + channel, flat );
		}

		public static async Task<List<IVideoInfo>> RetrieveVideosFromUser( string user, bool flat ) {
			List<IVideoInfo> videos = await RetrieveVideosFromParameterString( "ytuser:" + user, flat );

			// fetching a flat video list doesn't set in the username, but we know it since we used it to request the video list, so manually fill it in
			if ( flat ) {
				for ( int i = 0; i < videos.Count; ++i ) {
					videos[i].Username = user;
				}
			}

			return videos;
		}
	}
}
