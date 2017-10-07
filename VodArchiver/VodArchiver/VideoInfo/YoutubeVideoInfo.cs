using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

namespace VodArchiver.VideoInfo {
	[Serializable]
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

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", "YoutubeVideoInfo" );
			node.AppendAttribute( document, "username", Username );
			node.AppendAttribute( document, "videoId", VideoId );
			node.AppendAttribute( document, "videoTitle", VideoTitle );
			node.AppendAttribute( document, "videoTags", VideoGame );
			node.AppendAttribute( document, "videoTimestamp", VideoTimestamp.ToBinary().ToString() );
			node.AppendAttribute( document, "videoLength", VideoLength.TotalSeconds.ToString() );
			node.AppendAttribute( document, "videoRecordingState", VideoRecordingState.ToString() );
			node.AppendAttribute( document, "videoType", VideoType.ToString() );
			node.AppendAttribute( document, "userDisplayName", UserDisplayName );
			node.AppendAttribute( document, "videoDescription", VideoDescription );
			return node;
		}
	}
}
