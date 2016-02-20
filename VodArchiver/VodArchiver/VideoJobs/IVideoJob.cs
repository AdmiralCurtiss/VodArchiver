using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver.VideoJobs {
	public abstract class IVideoJob {
		private string _Status;
		public virtual string Status {
			get {
				return _Status;
			}
			set {
				_Status = value;
				StatusUpdater.Update();
			}
		}

		public virtual StatusUpdate.IStatusUpdate StatusUpdater { get; set; }
		
		public virtual VideoInfo.IVideoInfo VideoInfo { get; set; }

		public abstract Task Run();
	}
}
