using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver.VideoInfo {
	public class HitboxVideoInfo : IVideoInfo {
		HitboxVideo VideoInfo;

		public HitboxVideoInfo( HitboxVideo video ) {
			VideoInfo = video;
		}

		public StreamService Service {
			get {
				return StreamService.Hitbox;
			}

			set {
				throw new InvalidOperationException();
			}
		}

		public string Username {
			get {
				return VideoInfo.MediaUserName;
			}

			set {
				VideoInfo.MediaUserName = value;
			}
		}

		public string VideoGame {
			get {
				return VideoInfo.MediaGame;
			}

			set {
				VideoInfo.MediaGame = value;
			}
		}

		public string VideoId {
			get {
				return VideoInfo.MediaId.ToString();
			}

			set {
				VideoInfo.MediaId = int.Parse( value );
			}
		}

		public TimeSpan VideoLength {
			get {
				return TimeSpan.FromSeconds( VideoInfo.MediaDuration );
			}

			set {
				VideoInfo.MediaDuration = value.TotalSeconds;
			}
		}

		public RecordingState VideoRecordingState {
			get {
				return RecordingState.Recorded;
			}

			set {
				throw new InvalidOperationException();
			}
		}

		public DateTime VideoTimestamp {
			get {
				return VideoInfo.MediaDateAdded;
			}

			set {
				VideoInfo.MediaDateAdded = value;
			}
		}

		public string VideoTitle {
			get {
				return VideoInfo.MediaTitle;
			}

			set {
				VideoInfo.MediaTitle = value;
			}
		}

		public VideoFileType VideoType {
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
