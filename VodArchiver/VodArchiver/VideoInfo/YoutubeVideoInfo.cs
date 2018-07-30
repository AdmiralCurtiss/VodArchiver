using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

namespace VodArchiver.VideoInfo {
	public class YoutubeVideoInfo : IVideoInfo {
		public override StreamService Service {
			get {
				return StreamService.Youtube;
			}

			set {
				throw new InvalidOperationException();
			}
		}

		public string UserDisplayName { get; set; }
		public string VideoDescription { get; set; }

		public YoutubeVideoInfo() { }

		public YoutubeVideoInfo( XmlNode node ) {
			Username = node.Attributes["username"].Value;
			VideoId = node.Attributes["videoId"].Value;
			VideoTitle = node.Attributes["videoTitle"].Value;
			VideoGame = node.Attributes["videoTags"].Value;
			VideoTimestamp = DateTime.FromBinary( long.Parse( node.Attributes["videoTimestamp"].Value, Util.SerializationFormatProvider ) );
			VideoLength = TimeSpan.FromSeconds( double.Parse( node.Attributes["videoLength"].Value, Util.SerializationFormatProvider ) );
			VideoRecordingState = (RecordingState)Enum.Parse( typeof( RecordingState ), node.Attributes["videoRecordingState"].Value );
			VideoType = (VideoFileType)Enum.Parse( typeof( VideoFileType ), node.Attributes["videoType"].Value );
			UserDisplayName = node.Attributes["userDisplayName"].Value;
			VideoDescription = node.Attributes["videoDescription"].Value;
		}

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", "YoutubeVideoInfo" );
			node.AppendAttribute( document, "username", Username );
			node.AppendAttribute( document, "videoId", VideoId );
			node.AppendAttribute( document, "videoTitle", VideoTitle );
			node.AppendAttribute( document, "videoTags", VideoGame );
			node.AppendAttribute( document, "videoTimestamp", VideoTimestamp.ToBinary().ToString( Util.SerializationFormatProvider ) );
			node.AppendAttribute( document, "videoLength", VideoLength.TotalSeconds.ToString( Util.SerializationFormatProvider ) );
			node.AppendAttribute( document, "videoRecordingState", VideoRecordingState.ToString() );
			node.AppendAttribute( document, "videoType", VideoType.ToString() );
			node.AppendAttribute( document, "userDisplayName", UserDisplayName );
			node.AppendAttribute( document, "videoDescription", VideoDescription );
			return node;
		}
	}
}
