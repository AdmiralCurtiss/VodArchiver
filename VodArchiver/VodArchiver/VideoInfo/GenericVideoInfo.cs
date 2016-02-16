using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver.VideoInfo {
	public class GenericVideoInfo : IVideoInfo {
		public StreamService Service { get; set; }
		public string Username { get; set; }
		public string VideoGame { get; set; }
		public string VideoId { get; set; }
		public TimeSpan VideoLength { get; set; }
		public RecordingState VideoRecordingState { get; set; }
		public DateTime VideoTimestamp { get; set; }
		public string VideoTitle { get; set; }
		public VideoFileType VideoType { get; set; }
	}
}
