using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver.VideoJobs {
	public interface IVideoJob {
		string Status { get; set; }
		StatusUpdate.IStatusUpdate StatusUpdater { get; set; }
		VideoInfo.IVideoInfo VideoInfo { get; set; }

		Task Run();
	}
}
