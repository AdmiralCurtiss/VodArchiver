using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using VodArchiver.VideoInfo;

namespace VodArchiver {
	public class Youtube {
		public static YoutubeVideoInfo RetrieveVideo( string id ) {
			var data = Util.RunProgramSynchronous( @"youtube-dl", "-j \"https://www.youtube.com/watch?v=" + id + "\"" );
			var json = JObject.Parse( data.StdOut );

			YoutubeVideoInfo y = new YoutubeVideoInfo();
			y.Username = json["uploader_id"].ToString();
			y.VideoId = json["id"].ToString();
			y.VideoTitle = json["title"].ToString();

			var tags = json["tags"];
			if ( tags != null ) {
				StringBuilder sb = new StringBuilder();
				foreach ( var tag in tags ) {
					sb.Append( tag.ToString() ).Append( "; " );
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

			y.VideoTimestamp = DateTime.ParseExact( json["upload_date"].ToString(), "yyyyMMdd", null );
			y.VideoLength = TimeSpan.FromSeconds( (double)UInt64.Parse( json["duration"].ToString() ) );
			y.VideoRecordingState = RecordingState.Recorded;
			y.VideoType = VideoFileType.Unknown;

			y.UserDisplayName = json["uploader"].ToString();
			y.VideoDescription = json["description"].ToString();

			return y;
		}
	}
}
