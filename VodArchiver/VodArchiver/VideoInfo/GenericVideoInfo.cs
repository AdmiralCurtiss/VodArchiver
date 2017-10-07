using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

namespace VodArchiver.VideoInfo {
	[Serializable]
	public class GenericVideoInfo : IVideoInfo {
		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", "GenericVideoInfo" );
			node.AppendAttribute( document, "service", Service.ToString() );
			node.AppendAttribute( document, "username", Username );
			node.AppendAttribute( document, "videoId", VideoId );
			node.AppendAttribute( document, "videoTitle", VideoTitle );
			node.AppendAttribute( document, "videoTags", VideoGame );
			node.AppendAttribute( document, "videoTimestamp", VideoTimestamp.ToBinary().ToString() );
			node.AppendAttribute( document, "videoLength", VideoLength.TotalSeconds.ToString() );
			node.AppendAttribute( document, "videoRecordingState", VideoRecordingState.ToString() );
			node.AppendAttribute( document, "videoType", VideoType.ToString() );
			return node;
		}
	}
}
