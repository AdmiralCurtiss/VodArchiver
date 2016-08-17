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

		public virtual string HumanReadableJobName {
			get {
				if ( VideoInfo != null ) {
					if ( !String.IsNullOrEmpty( VideoInfo.Username ) ) {
						return VideoInfo.Username + "/" + VideoInfo.VideoId + " (" + VideoInfo.Service + ")";
					} else {
						return VideoInfo.VideoId + " (" + VideoInfo.Service + ")";
					}
				} else {
					return "Unknown Video";
				}
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

		[System.Runtime.Serialization.OptionalField( VersionAdded = 2 )]
		public bool HasBeenValidated;

		[System.Runtime.Serialization.OptionalField( VersionAdded = 3 )]
		public string Notes;

		public abstract Task Run();
	}
}
