using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver.VideoJobs {
	public enum VideoJobStatus {
		NotStarted,
		Running,
		Finished,
	}

	[Serializable]
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

		[NonSerialized]
		public StatusUpdate.IStatusUpdate StatusUpdater;

		public VideoJobStatus JobStatus;

		public string ButtonAction {
			get {
				switch ( JobStatus ) {
					case VideoJobStatus.NotStarted: return "Force Start";
					default: return null;
				}
			}
		}
		
		public virtual VideoInfo.IVideoInfo VideoInfo { get; set; }

		public abstract Task Run();
	}
}
