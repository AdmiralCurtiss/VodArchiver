using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver.VideoInfo {
	class TwitchVideoInfo : IVideoInfo {
		TwixelAPI.Video VideoInfo;

		public TwitchVideoInfo( TwixelAPI.Video video ) {
			VideoInfo = video;
		}

		public StreamService Service {
			get {
				return StreamService.Twitch;
			}

			set {
				throw new InvalidOperationException();
			}
		}

		public string Username {
			get {
				return VideoInfo.channel["name"];
			}

			set {
				VideoInfo.channel["name"] = value;
			}
		}

		public string VideoGame {
			get {
				return VideoInfo.game;
			}

			set {
				VideoInfo.game = value;
			}
		}

		public string VideoId {
			get {
				return VideoInfo.id;
			}

			set {
				VideoInfo.id = value;
			}
		}

		public TimeSpan VideoLength {
			get {
				return TimeSpan.FromSeconds( VideoInfo.length );
			}

			set {
				VideoInfo.length = (long)value.TotalSeconds;
			}
		}

		public DateTime VideoTimestamp {
			get {
				return VideoInfo.recordedAt;
			}

			set {
				VideoInfo.recordedAt = value;
			}
		}

		public string VideoTitle {
			get {
				return VideoInfo.title;
			}

			set {
				VideoInfo.title = value;
			}
		}
	}
}
