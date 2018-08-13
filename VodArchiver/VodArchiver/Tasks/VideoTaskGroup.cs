using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using VodArchiver.VideoInfo;
using VodArchiver.VideoJobs;

namespace VodArchiver.Tasks {
	public class WaitingVideoJob {
		public IVideoJob Job { get; private set; }
		public bool StartImmediately { get; set; }
		public DateTime EarliestPossibleStartTime { get; set; }

		public WaitingVideoJob( IVideoJob job, bool startImmediately = false ) : this( job, DateTime.UtcNow, startImmediately ) { }
		public WaitingVideoJob( IVideoJob job, DateTime allowedStartTime, bool startImmediately = false ) {
			Job = job;
			StartImmediately = startImmediately;
			EarliestPossibleStartTime = allowedStartTime;
		}
		public bool IsAllowedToStart() {
			return DateTime.UtcNow >= EarliestPossibleStartTime && !Job.IsWaitingForUserInput;
		}
	}

	public class VideoTaskGroup {
		public delegate void RequestSaveJobsDelegate();
		public delegate void RequestPowerEventDelegate();

		private StreamService Service;
		private List<WaitingVideoJob> WaitingJobs;
		private object JobQueueLock;
		private int MaxJobsRunningPerType;
		private CancellationToken CancellationToken;

		private Task JobRunnerThread;
		private List<(IVideoJob job, Task<ResultType> task, CancellationTokenSource cancellationTokenSource)> RunningTasks;

		private RequestSaveJobsDelegate RequestSaveJobs;
		private RequestPowerEventDelegate RequestPowerEvent;

		public VideoTaskGroup( StreamService service, RequestSaveJobsDelegate saveJobsDelegate, RequestPowerEventDelegate powerEventDelegate, CancellationToken cancellationToken ) {
			Service = service;
			WaitingJobs = new List<WaitingVideoJob>();
			JobQueueLock = new object();
			MaxJobsRunningPerType = ( service == StreamService.Youtube || service == StreamService.RawUrl || service == StreamService.FFMpegJob ) ? 1 : 3;
			RunningTasks = new List<(IVideoJob job, Task<ResultType> task, CancellationTokenSource cancellationTokenSource)>( MaxJobsRunningPerType );
			RequestSaveJobs = saveJobsDelegate;
			RequestPowerEvent = powerEventDelegate;
			CancellationToken = cancellationToken;

			JobRunnerThread = RunJobRunnerThread();
		}

		public void Add( WaitingVideoJob wj ) {
			lock ( JobQueueLock ) {
				if ( !CancellationToken.IsCancellationRequested ) {
					if ( IsJobWaiting( wj.Job ) ) {
						WaitingVideoJob alreadyEnqueuedJob = WaitingJobs.Find( x => x.Job == wj.Job );
						if ( alreadyEnqueuedJob != null ) {
							alreadyEnqueuedJob.EarliestPossibleStartTime = wj.EarliestPossibleStartTime;
							alreadyEnqueuedJob.StartImmediately = wj.StartImmediately;
						}
					} else if ( !IsJobRunning( wj.Job ) ) {
						WaitingJobs.Add( wj );
					}
				}
			}
		}

		public bool IsEmpty() {
			lock ( JobQueueLock ) {
				return WaitingJobs.Count == 0 && RunningTasks.Count == 0;
			}
		}

		private IVideoJob DequeueVideoJobForTask() {
			lock ( JobQueueLock ) {
				WaitingVideoJob waitingJob = null;
				foreach ( WaitingVideoJob wj in WaitingJobs ) {
					if ( wj.StartImmediately || ( RunningTasks.Count < MaxJobsRunningPerType && wj.IsAllowedToStart() ) ) {
						waitingJob = wj;
						break;
					}
				}

				if ( waitingJob != null ) {
					WaitingJobs.Remove( waitingJob );
					return waitingJob.Job;
				}

				return null;
			}
		}

		private void ProcessFinishedTasks() {
			lock ( JobQueueLock ) {
				for ( int i = RunningTasks.Count - 1; i >= 0; --i ) {
					(IVideoJob taskJob, Task<ResultType> task, CancellationTokenSource cancellationTokenSource) = RunningTasks[i];
					if ( task.IsCompleted ) {
						// need to remove the task first, as otherwise the Add() still detects it as running
						RunningTasks.RemoveAt( i );

						if ( task.Status == TaskStatus.RanToCompletion ) {
							ResultType result = task.Result;
							if ( result == ResultType.UserInputRequired ) { 
								Add( new WaitingVideoJob( taskJob ) );
							}
							if ( result == ResultType.TemporarilyUnavailable ) {
								Add( new WaitingVideoJob( taskJob, DateTime.UtcNow.AddMinutes( 30.0 ) ) );
							}
						} else {
							if ( task.Exception != null ) {
								taskJob.Status = "Failed via unexpected exception: " + task.Exception.ToString();
							} else {
								taskJob.Status = "Failed for unknown reasons.";
							}
						}

						RequestSaveJobs();
						RequestPowerEvent();
					}
				}
			}
		}

		private bool IsJobWaiting( IVideoJob job ) {
			lock ( JobQueueLock ) {
				return WaitingJobs.Any( x => x.Job == job );
			}
		}

		private bool IsJobRunning( IVideoJob job ) {
			lock ( JobQueueLock ) {
				return RunningTasks.Any( x => x.job == job );
			}
		}

		private bool IsJobWaitingOrRunning( IVideoJob job ) {
			lock ( JobQueueLock ) {
				return IsJobWaiting( job ) || IsJobRunning( job );
			}
		}

		private async Task RunJobRunnerThread() {
			while ( true ) {
				if ( CancellationToken.IsCancellationRequested ) {
					lock ( JobQueueLock ) {
						if ( RunningTasks.Count == 0 ) {
							break;
						}
					}
				}

				try {
					if ( !CancellationToken.IsCancellationRequested ) {
						await Task.Delay( 750, CancellationToken );
					} else {
						await Task.Delay( 50 );
					}
				} catch ( Exception ) { }

				try {
					if ( !CancellationToken.IsCancellationRequested ) {
						lock ( JobQueueLock ) {
							// find jobs we can start
							IVideoJob job = null;
							while ( ( job = DequeueVideoJobForTask() ) != null ) {
								// and run them
								CancellationTokenSource cts = new CancellationTokenSource();
								RunningTasks.Add( (job, RunJob( job, cts.Token ), cts) );
							}
						}
					} else {
						lock ( JobQueueLock ) {
							// stop all running tasks
							foreach ( var t in RunningTasks ) {
								t.cancellationTokenSource.Cancel();
							}
						}
					}
				} catch ( Exception ) { }

				// TODO: murder tasks that are unresponsive

				try {
					lock ( JobQueueLock ) {
						// see if any tasks finished and do cleanup work
						ProcessFinishedTasks();
					}
				} catch ( Exception ) { }
			}
		}

		private static async Task<ResultType> RunJob( IVideoJob job, CancellationToken cancellationToken ) {
			if ( job != null ) {
				bool wasDead = job.JobStatus == VideoJobStatus.Dead;
				try {
					if ( job.JobStatus != VideoJobStatus.Finished ) {
						job.JobStartTimestamp = DateTime.UtcNow;
						ResultType result = await job.Run( cancellationToken );
						if ( result == ResultType.Success ) {
							job.JobFinishTimestamp = DateTime.UtcNow;
							job.JobStatus = VideoJobStatus.Finished;
						} else if ( result == ResultType.Dead ) {
							job.Status = "Dead.";
							job.JobStatus = VideoJobStatus.Dead;
						} else {
							job.JobStatus = wasDead ? VideoJobStatus.Dead : VideoJobStatus.NotStarted;
						}

						if ( result == ResultType.Cancelled ) {
							job.Status = "Cancelled during: " + job.Status;
						}

						if ( result == ResultType.TemporarilyUnavailable ) {
							job.Status = "Temporarily unavailable, retrying later.";
						}

						if ( Util.ShowToastNotifications ) {
							if ( result == ResultType.Success ) {
								ToastUtil.ShowToast( "Downloaded " + job.HumanReadableJobName + "!" );
							} else if ( result == ResultType.Failure ) {
								ToastUtil.ShowToast( "Failed to download " + job.HumanReadableJobName + "." );
							}
						}

						return result;
					} else {
						// task is already finished, no need to do anything
						return ResultType.Success;
					}
				} catch ( Exception ex ) {
					job.JobStatus = wasDead ? VideoJobStatus.Dead : VideoJobStatus.NotStarted;
					job.Status = "ERROR: " + ex.ToString();
					if ( Util.ShowToastNotifications ) {
						ToastUtil.ShowToast( "Failed to download " + job.HumanReadableJobName + ": " + ex.ToString() );
					}

					return ResultType.Failure;
				}
			}

			return ResultType.Failure;
		}

		public async Task WaitForJobRunnerThreadToEnd() {
			await JobRunnerThread;
		}

		public bool CancelJob( IVideoJob job ) {
			lock ( JobQueueLock ) {
				bool cancelled = false;
				foreach ( var rt in RunningTasks.Where( x => x.job == job ) ) {
					rt.cancellationTokenSource.Cancel();
					cancelled = true;
				}
				return cancelled;
			}
		}

		public bool IsInQueue( IVideoJob job ) {
			lock ( JobQueueLock ) {
				return IsJobWaiting( job );
			}
		}

		public bool Dequeue( IVideoJob job ) {
			lock ( JobQueueLock ) {
				return WaitingJobs.RemoveAll( x => x.Job == job ) > 0;
			}
		}
	}
}
