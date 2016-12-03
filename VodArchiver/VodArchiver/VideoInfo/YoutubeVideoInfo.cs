using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver.VideoInfo {
	[Serializable]
	public class YoutubeVideoInfo : IVideoInfo {
		public override StreamService Service {
			get {
				return StreamService.Youtube;
			}

			set {
				throw new InvalidOperationException();
			}
		}

		public string UserDisplayName { get; set; }
		public string VideoDescription { get; set; }
	}
}
