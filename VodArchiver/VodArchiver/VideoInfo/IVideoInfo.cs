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

	[Serializable]
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
