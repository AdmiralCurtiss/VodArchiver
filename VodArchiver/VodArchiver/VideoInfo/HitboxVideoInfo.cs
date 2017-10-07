using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

namespace VodArchiver.VideoInfo {
	[Serializable]
	public class HitboxVideoInfo : IVideoInfo {
		HitboxVideo VideoInfo;

		public HitboxVideoInfo( HitboxVideo video ) {
			VideoInfo = video;
		}

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", "HitboxVideoInfo" );
			node.AppendChild( VideoInfo.Serialize( document, document.CreateElement( "VideoInfo" ) ) );
			return node;
		}

		public override StreamService Service {
			get {
				return StreamService.Hitbox;
			}

			set {
				throw new InvalidOperationException();
			}
		}

		public override string Username {
			get {
				return VideoInfo.MediaUserName;
			}

			set {
				VideoInfo.MediaUserName = value;
			}
		}

		public override string VideoGame {
			get {
				return VideoInfo.MediaGame;
			}

			set {
				VideoInfo.MediaGame = value;
			}
		}

		public override string VideoId {
			get {
				return VideoInfo.MediaId.ToString();
			}

			set {
				VideoInfo.MediaId = int.Parse( value );
			}
		}

		public override TimeSpan VideoLength {
			get {
				return TimeSpan.FromSeconds( VideoInfo.MediaDuration );
			}

			set {
				VideoInfo.MediaDuration = value.TotalSeconds;
			}
		}

		public override RecordingState VideoRecordingState {
			get {
				return RecordingState.Recorded;
			}

			set {
				throw new InvalidOperationException();
			}
		}

		public override DateTime VideoTimestamp {
			get {
				return VideoInfo.MediaDateAdded;
			}

			set {
				VideoInfo.MediaDateAdded = value;
			}
		}

		public override string VideoTitle {
			get {
				return VideoInfo.MediaTitle;
			}

			set {
				VideoInfo.MediaTitle = value;
			}
		}

		public override VideoFileType VideoType {
			get {
				if ( VideoInfo.MediaProfiles.First().Url.EndsWith( "m3u8" ) ) {
					return VideoFileType.M3U;
				}

				return VideoFileType.Unknown;
			}

			set {
				throw new InvalidOperationException();
			}
		}
	}
}
