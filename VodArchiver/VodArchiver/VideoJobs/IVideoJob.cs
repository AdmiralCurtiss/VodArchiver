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
		string Status { get; set; }

		Task Run();
	}
}
