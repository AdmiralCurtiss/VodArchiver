using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver.VideoJobs {
	interface IVideoJob {
		Task Run();
	}
}
