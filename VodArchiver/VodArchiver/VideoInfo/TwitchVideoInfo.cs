﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver.VideoInfo {
	[Serializable]
	public class TwitchVideoInfo : IVideoInfo {
		TwixelAPI.Video VideoInfo;

		[System.Runtime.Serialization.OptionalField( VersionAdded = 2 )]
		StreamService _Service;

		public TwitchVideoInfo( TwixelAPI.Video video, StreamService service = StreamService.Twitch ) {
			VideoInfo = video;
			_Service = service;
		}

		public override StreamService Service {
			get {
				return _Service;
			}

			set {
				_Service = value;
			}
		}

		public override string Username {
			get {
				return VideoInfo.channel["name"];
			}

			set {
				VideoInfo.channel["name"] = value;
			}
		}

		public override string VideoGame {
			get {
				return VideoInfo.game;
			}

			set {
				VideoInfo.game = value;
			}
		}

		public override string VideoId {
			get {
				return VideoInfo.id;
			}

			set {
				VideoInfo.id = value;
			}
		}

		public override TimeSpan VideoLength {
			get {
				return TimeSpan.FromSeconds( VideoInfo.length );
			}

			set {
				VideoInfo.length = (long)value.TotalSeconds;
			}
		}

		public override RecordingState VideoRecordingState {
			get {
				switch ( VideoInfo.status ) {
					case "recording": return RecordingState.Live;
					case "recorded": return RecordingState.Recorded;
					default: return RecordingState.Unknown;
				}
			}

			set {
				switch ( value ) {
					case RecordingState.Live: VideoInfo.status = "recording"; break;
					case RecordingState.Recorded: VideoInfo.status = "recorded"; break;
					default: VideoInfo.status = "?"; break;
				}
			}
		}

		public override DateTime VideoTimestamp {
			get {
				return VideoInfo.recordedAt;
			}

			set {
				VideoInfo.recordedAt = value;
			}
		}

		public override string VideoTitle {
			get {
				return VideoInfo.title;
			}

			set {
				VideoInfo.title = value;
			}
		}

		public override VideoFileType VideoType {
			get {
				if ( VideoId.StartsWith( "v" ) ) { return VideoFileType.M3U; }
				if ( VideoId.StartsWith( "a" ) ) { return VideoFileType.FLVs; }
				return VideoFileType.Unknown;
			}

			set {
				throw new InvalidOperationException();
			}
		}

		[System.Runtime.Serialization.OnDeserializing]
		private void SetCountryRegionDefault( System.Runtime.Serialization.StreamingContext sc ) {
			_Service = StreamService.Twitch;
		}
	}
}
