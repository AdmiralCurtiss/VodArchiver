using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver.VideoJobs {
	interface IVideoJob {
		string ServiceName { get; set; }
		string Username { get; set; }
		string VideoId { get; set; }
		string VideoTitle { get; set; }
		string VideoGame { get; set; }
		DateTime VideoTimestamp { get; set; }
		TimeSpan VideoLength { get; set; }
		string Status { get; set; }

		StatusUpdate.IStatusUpdate StatusUpdater { get; set; }

		Task Run();
	}
}
