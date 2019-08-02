using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

namespace VodArchiver.VideoInfo {
	public enum StreamService {
		Unknown,
		Twitch,
		Hitbox,
		TwitchChatReplay,
		Youtube,
		RawUrl,
		FFMpegJob,
	}

	public enum RecordingState {
		Unknown,
		Live,
		Recorded,
	}

	public enum VideoFileType {
		M3U,
		FLVs,
		Unknown
	}

	public abstract class IVideoInfo {
		public virtual StreamService Service { get; set; }
		public virtual string Username { get; set; }
		public virtual string VideoId { get; set; }
		public virtual string VideoTitle { get; set; }
		public virtual string VideoGame { get; set; }
		public virtual DateTime VideoTimestamp { get; set; }
		public virtual TimeSpan VideoLength { get; set; }
		public virtual RecordingState VideoRecordingState { get; set; }
		public virtual VideoFileType VideoType { get; set; }

		public abstract XmlNode Serialize( XmlDocument document, XmlNode node );

		public static IVideoInfo Deserialize( XmlNode node ) {
			string type = node.Attributes["_type"].Value;
			switch ( type ) {
				case "GenericVideoInfo": return new GenericVideoInfo( node );
				case "HitboxVideoInfo": return new HitboxVideoInfo( node );
				case "TwitchVideoInfo": return new TwitchTwixelVideoInfo( node );
				case "YoutubeVideoInfo": return new YoutubeVideoInfo( node );
				case "FFMpegReencodeJobVideoInfo": return new FFMpegReencodeJobVideoInfo( node );
				default: throw new Exception( "Unknown video job type: " + type );
			}
		}

		public override bool Equals( object obj ) {
			return Equals( obj as IVideoInfo );
		}

		public bool Equals( IVideoInfo other ) {
			return other != null && Service == other.Service && VideoId == other.VideoId;
		}

		public override int GetHashCode() {
			return Service.GetHashCode() ^ VideoId.GetHashCode();
		}
	}
}
