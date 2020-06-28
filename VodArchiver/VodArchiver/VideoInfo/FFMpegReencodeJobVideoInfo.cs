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
		public override string Username { get => PostfixNew; set => throw new Exception(); }
		public override string VideoGame { get => String.Format( "{0:#,#} MB; {1:#,#} kbps", Filesize / 1000000, Bitrate / 1000 ); set => throw new Exception(); }

		public List<string> FFMpegOptions;
		public string PostfixOld;
		public string PostfixNew;
		public string OutputFormat;

		private ulong Filesize;
		private ulong Bitrate;

		public FFMpegReencodeJobVideoInfo( string filename, FFProbeResult probe, List<string> ffmpegOptions, string postfixOld, string postfixNew, string outputformat ) {
			VideoTitle = System.IO.Path.GetFileNameWithoutExtension( filename );
			VideoId = System.IO.Path.GetFullPath( filename );
			Filesize = probe.Filesize;
			Bitrate = probe.Bitrate;
			VideoTimestamp = probe.Timestamp;
			VideoLength = probe.Duration;
			FFMpegOptions = ffmpegOptions;
			PostfixOld = postfixOld;
			PostfixNew = postfixNew;
			OutputFormat = outputformat;
		}

		public FFMpegReencodeJobVideoInfo( XmlNode node ) {
			VideoTitle = node.Attributes["username"].Value;
			VideoId = node.Attributes["videoId"].Value;
			try {
				Filesize = ulong.Parse( node.Attributes["filesize"].Value, Util.SerializationFormatProvider );
				Bitrate = ulong.Parse( node.Attributes["bitrate"].Value, Util.SerializationFormatProvider );
			} catch ( Exception ) {
				try {
					// old format just stored these directly as a string, so try parsing that
					string[] tags = node.Attributes["videoTags"].Value.Split( ';' );
					Filesize = (ulong)( double.Parse( tags[0].Trim().Split( ' ' )[0].Trim() ) * 1000000 );
					Bitrate = (ulong)( double.Parse( tags[1].Trim().Split( ' ' )[0].Trim() ) * 1000 );
				} catch ( Exception ) {
					Filesize = 0;
					Bitrate = 0;
				}
			}
			VideoTimestamp = DateTime.FromBinary( long.Parse( node.Attributes["videoTimestamp"].Value, Util.SerializationFormatProvider ) );
			try {
				VideoLength = TimeSpan.FromSeconds( double.Parse( node.Attributes["videoLength"].Value, Util.SerializationFormatProvider ) );
			} catch ( OverflowException ) {
				VideoLength = TimeSpan.MaxValue;
			}
			PostfixOld = node.Attributes["postfixOld"].Value;
			PostfixNew = node.Attributes["postfixNew"].Value;
			OutputFormat = node?.Attributes["outputFormat"]?.Value ?? "mp4";
			FFMpegOptions = new List<string>();
			foreach ( XmlNode n in node.ChildNodes ) {
				if ( n.Name == "FFMpegOption" ) {
					FFMpegOptions.Add( n.Attributes["arg"].Value );
				}
			}
		}

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", "FFMpegReencodeJobVideoInfo" );
			node.AppendAttribute( document, "username", VideoTitle );
			node.AppendAttribute( document, "videoId", VideoId );
			node.AppendAttribute( document, "filesize", Filesize.ToString( Util.SerializationFormatProvider ) );
			node.AppendAttribute( document, "bitrate", Bitrate.ToString( Util.SerializationFormatProvider ) );
			node.AppendAttribute( document, "videoTimestamp", VideoTimestamp.ToBinary().ToString( Util.SerializationFormatProvider ) );
			node.AppendAttribute( document, "videoLength", VideoLength.TotalSeconds.ToString( Util.SerializationFormatProvider ) );
			node.AppendAttribute( document, "postfixOld", PostfixOld );
			node.AppendAttribute( document, "postfixNew", PostfixNew );
			node.AppendAttribute( document, "outputFormat", OutputFormat );
			foreach ( string option in FFMpegOptions ) {
				var ffmpegOptionsNode = document.CreateElement( "FFMpegOption" );
				ffmpegOptionsNode.AppendAttribute( document, "arg", option );
				node.AppendChild( ffmpegOptionsNode );
			}
			return node;
		}
	}
}
