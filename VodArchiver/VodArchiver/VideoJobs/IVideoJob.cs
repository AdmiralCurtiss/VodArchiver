﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver.VideoJobs {
	public enum VideoJobStatus {
		NotStarted,
		Running,
		Finished,
		Dead,
	}

	public enum ResultType {
		Failure,
		Success,
		Cancelled,
		Dead,
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

		[NonSerialized]
		protected bool Stopped;

		[NonSerialized]
		public long Index;

		public VideoJobStatus JobStatus;

		public virtual VideoInfo.IVideoInfo VideoInfo { get; set; }

		[System.Runtime.Serialization.OptionalField( VersionAdded = 2 )]
		public bool HasBeenValidated;

		[System.Runtime.Serialization.OptionalField( VersionAdded = 3 )]
		public string Notes;

		[System.Runtime.Serialization.OptionalField( VersionAdded = 4 )]
		public DateTime JobStartTimestamp;

		[System.Runtime.Serialization.OptionalField( VersionAdded = 4 )]
		public DateTime JobFinishTimestamp;

		public abstract Task<ResultType> Run();

		public override bool Equals( object obj ) {
			return Equals( obj as IVideoJob );
		}

		public bool Equals( IVideoJob other ) {
			return other != null && VideoInfo.Equals( other.VideoInfo );
		}

		public override int GetHashCode() {
			return VideoInfo.GetHashCode();
		}

		public void Stop() {
			Stopped = true;
		}

		public bool IsStopped() {
			return Stopped;
		}
	}
}
