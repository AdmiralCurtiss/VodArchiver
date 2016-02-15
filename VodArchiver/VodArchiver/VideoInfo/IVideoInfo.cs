using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver.VideoInfo {
	public enum StreamService {
		Unknown,
		Twitch,
		Hitbox
	}

	public enum RecordingState {
		Unknown,
		Live,
		Recorded,
	}

	public interface IVideoInfo {
		StreamService Service { get; set; }
		string Username { get; set; }
		string VideoId { get; set; }
		string VideoTitle { get; set; }
		string VideoGame { get; set; }
		DateTime VideoTimestamp { get; set; }
		TimeSpan VideoLength { get; set; }
		RecordingState VideoRecordingState { get; set; }
	}
}
