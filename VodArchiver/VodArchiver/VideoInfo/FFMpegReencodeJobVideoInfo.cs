using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

namespace VodArchiver.VideoInfo {
	public class FFMpegReencodeJobVideoInfo : IVideoInfo {
		public override StreamService Service { get => StreamService.FFMpegJob; set => throw new Exception(); }
		public override RecordingState VideoRecordingState { get => RecordingState.Recorded; set => throw new Exception(); }
		public override VideoFileType VideoType { get => VideoFileType.Unknown; set => throw new Exception(); }
		public override string VideoTitle { get => Username; set => throw new Exception(); }

		public List<string> FFMpegOptions;
		public string PostfixOld;
		public string PostfixNew;

		public FFMpegReencodeJobVideoInfo( GenericVideoInfo videoInfo, List<string> ffmpegOptions, string postfixOld, string postfixNew ) {
			Username = videoInfo.Username;
			VideoId = videoInfo.VideoId;
			VideoGame = videoInfo.VideoGame;
			VideoTimestamp = videoInfo.VideoTimestamp;
			VideoLength = videoInfo.VideoLength;
			FFMpegOptions = ffmpegOptions;
			PostfixOld = postfixOld;
			PostfixNew = postfixNew;
		}

		public FFMpegReencodeJobVideoInfo( XmlNode node ) {
			Username = node.Attributes["username"].Value;
			VideoId = node.Attributes["videoId"].Value;
			VideoGame = node.Attributes["videoTags"].Value;
			VideoTimestamp = DateTime.FromBinary( long.Parse( node.Attributes["videoTimestamp"].Value ) );
			try {
				VideoLength = TimeSpan.FromSeconds( double.Parse( node.Attributes["videoLength"].Value ) );
			} catch ( OverflowException ) {
				VideoLength = TimeSpan.MaxValue;
			}
			PostfixOld = node.Attributes["postfixOld"].Value;
			PostfixNew = node.Attributes["postfixNew"].Value;
			FFMpegOptions = new List<string>();
			foreach ( XmlNode n in node.ChildNodes ) {
				if ( n.Name == "FFMpegOption" ) {
					FFMpegOptions.Add( n.Attributes["arg"].Value );
				}
			}
		}

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", "FFMpegReencodeJobVideoInfo" );
			node.AppendAttribute( document, "username", Username );
			node.AppendAttribute( document, "videoId", VideoId );
			node.AppendAttribute( document, "videoTags", VideoGame );
			node.AppendAttribute( document, "videoTimestamp", VideoTimestamp.ToBinary().ToString() );
			node.AppendAttribute( document, "videoLength", VideoLength.TotalSeconds.ToString() );
			node.AppendAttribute( document, "postfixOld", PostfixOld );
			node.AppendAttribute( document, "postfixNew", PostfixNew );
			foreach ( string option in FFMpegOptions ) {
				var ffmpegOptionsNode = document.CreateElement( "FFMpegOption" );
				ffmpegOptionsNode.AppendAttribute( document, "arg", option );
				node.AppendChild( ffmpegOptionsNode );
			}
			return node;
		}
	}
}
