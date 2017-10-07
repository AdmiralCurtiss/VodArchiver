using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

namespace VodArchiver.VideoInfo {
	[Serializable]
	public class GenericVideoInfo : IVideoInfo {
		public GenericVideoInfo() { }

		public GenericVideoInfo( XmlNode node ) {
			Service = (StreamService)Enum.Parse( typeof( StreamService ), node.Attributes["service"].Value );
			Username = node.Attributes["username"].Value;
			VideoId = node.Attributes["videoId"].Value;
			VideoTitle = node.Attributes["videoTitle"].Value;
			VideoGame = node.Attributes["videoTags"].Value;
			VideoTimestamp = DateTime.FromBinary( long.Parse( node.Attributes["videoTimestamp"].Value ) );
			VideoLength = TimeSpan.FromSeconds( double.Parse( node.Attributes["videoLength"].Value ) );
			VideoRecordingState = (RecordingState)Enum.Parse( typeof( RecordingState ), node.Attributes["videoRecordingState"].Value );
			VideoType = (VideoFileType)Enum.Parse( typeof( VideoFileType ), node.Attributes["videoType"].Value );
		}

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
