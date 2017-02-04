﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using VodArchiver.VideoInfo;
using VodArchiver.VideoJobs;

namespace VodArchiver.Tasks {
	public class WaitingVideoJob {
		public IVideoJob Job;
		public DateTime EarliestPossibleStartTime;

		public WaitingVideoJob( IVideoJob job ) { Job = job; EarliestPossibleStartTime = DateTime.UtcNow; }
		public WaitingVideoJob( IVideoJob job, DateTime allowedStartTime ) { Job = job; EarliestPossibleStartTime = allowedStartTime; }
		public bool IsAllowedToStart() {
			return DateTime.UtcNow >= EarliestPossibleStartTime;
		}
	}

	public class VideoTaskGroup {
		// TODO: Get rid of this reference. The handful of calls we need to it should be via delegates or something.
		private DownloadForm Parent;

		private StreamService Service;
		private List<WaitingVideoJob> WaitingJobs;
		private int JobsRunningPerType;
		private object JobQueueLock;
		private const int MaxJobsRunningPerType = 1;

		public VideoTaskGroup( DownloadForm parent, StreamService service ) {
			Parent = parent;
			Service = service;
			WaitingJobs = new List<WaitingVideoJob>();
			JobsRunningPerType = 0;
			JobQueueLock = new object();
		}

		public void Add( WaitingVideoJob job ) {
			lock ( JobQueueLock ) {
				WaitingJobs.Add( job );
			}
		}

		public bool IsEmpty() {
			lock ( JobQueueLock ) {
				return WaitingJobs.Count == 0 && JobsRunningPerType == 0;
			}
		}

		public async Task RunJob( IVideoJob job = null, bool forceStart = false ) {
			bool runNewJob = false;
			lock ( JobQueueLock ) {
				// TODO: This almost certainly has odd results if one thread tries to dequeue a job that was force-started from somewhere else!
				if ( forceStart || JobsRunningPerType < MaxJobsRunningPerType ) {
					if ( job != null ) {
						runNewJob = true;
						JobsRunningPerType += 1;
					} else {
						if ( WaitingJobs.Count != 0 ) {
							WaitingVideoJob waitingJob = null;
							foreach ( WaitingVideoJob wj in WaitingJobs ) {
								if ( wj.IsAllowedToStart() ) {
									waitingJob = wj;
									break;
								}
							}
							if ( waitingJob != null ) {
								job = waitingJob.Job;
								WaitingJobs.Remove( waitingJob );
								runNewJob = true;
								JobsRunningPerType += 1;
							}
						}
					}
				}
			}

			if ( runNewJob ) {
				try {
					if ( job.JobStatus != VideoJobStatus.Finished && job.JobStatus != VideoJobStatus.Dead ) {
						job.JobStartTimestamp = DateTime.UtcNow;
						await job.Run();
						job.JobFinishTimestamp = DateTime.UtcNow;
						if ( Util.ShowToastNotifications ) {
							ToastUtil.ShowToast( "Downloaded " + job.HumanReadableJobName + "!" );
						}
					}
				} catch ( RetryLaterException ex ) {
					job.JobStatus = VideoJobStatus.NotStarted;
					job.Status = "Retry Later: " + ex.ToString();
					lock ( JobQueueLock ) {
						WaitingJobs.Add( new WaitingVideoJob( job, DateTime.UtcNow.AddMinutes( 10.0 ) ) );
					}
				} catch ( VideoDeadException ex ) {
					job.JobStatus = VideoJobStatus.Dead;
					job.Status = ex.Message;
				} catch ( Exception ex ) {
					job.JobStatus = VideoJobStatus.NotStarted;
					job.Status = "ERROR: " + ex.ToString();
					if ( Util.ShowToastNotifications ) {
						ToastUtil.ShowToast( "Failed to download " + job.HumanReadableJobName + ": " + ex.ToString() );
					}
				} finally {
					lock ( JobQueueLock ) {
						JobsRunningPerType -= 1;
					}
				}

				Parent.InvokeSaveJobs();
				Parent.InvokePowerEvent();

				await RunJob();
			}
		}
	}
}