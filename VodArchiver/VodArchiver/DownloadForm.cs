﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Net.Http;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using TwixelAPI;
using VodArchiver.VideoInfo;
using VodArchiver.VideoJobs;

namespace VodArchiver {
	public class WaitingVideoJob {
		public IVideoJob Job;
		public DateTime EarliestPossibleStartTime;

		public WaitingVideoJob( IVideoJob job ) { Job = job; EarliestPossibleStartTime = DateTime.UtcNow; }
		public WaitingVideoJob( IVideoJob job, DateTime allowedStartTime ) { Job = job; EarliestPossibleStartTime = allowedStartTime; }
		public bool IsAllowedToStart() {
			return DateTime.UtcNow >= EarliestPossibleStartTime;
		}
	}

	public partial class DownloadForm : Form {
		Twixel TwitchAPI;

		HashSet<IVideoJob> JobSet;

		Dictionary<StreamService, List<WaitingVideoJob>> WaitingJobs;
		Dictionary<StreamService, int> JobsRunningPerType;
		private object JobQueueLock = new object();
		public const int MaxJobsRunningPerType = 1;

		Task TimedAutoFetchTask;

		public DownloadForm() {
			InitializeComponent();
			objectListViewDownloads.CellEditActivation = BrightIdeasSoftware.ObjectListView.CellEditActivateMode.DoubleClick;

			comboBoxService.SelectedIndex = 0;
			comboBoxPowerStateWhenDone.SelectedIndex = 0;
			TwitchAPI = new Twixel( Util.TwitchClientId, Util.TwitchRedirectURI, Twixel.APIVersion.v3 );

			JobSet = new HashSet<IVideoJob>();

			WaitingJobs = new Dictionary<StreamService, List<WaitingVideoJob>>();
			JobsRunningPerType = new Dictionary<StreamService, int>();
			foreach ( StreamService s in Enum.GetValues( typeof( StreamService ) ) ) {
				WaitingJobs.Add( s, new List<WaitingVideoJob>() );
				JobsRunningPerType.Add( s, 0 );
			}

			if ( !Util.AllowTimedAutoFetch ) {
				labelStatusBar.Hide();
			}

			objectListViewDownloads.SecondarySortColumn = objectListViewDownloads.GetColumn( "Video ID" );
			objectListViewDownloads.SecondarySortOrder = SortOrder.Ascending;

			columnStatus.GroupKeyGetter = delegate ( object rowObject ) {
				return ( (IVideoJob)rowObject ).JobStatus;
			};
			columnIndex.GroupKeyGetter = delegate ( object rowObject ) {
				return 0;
			};

			objectListViewDownloads.FormatRow += ObjectListViewDownloads_FormatRow;

			LoadJobs();

			if ( Util.AllowTimedAutoFetch ) {
				StartTimedAutoFetch();
			}
		}

		private void StartTimedAutoFetch() {
			TimedAutoFetchTask = RunTimedAutoFetch();
		}

		private async Task RunTimedAutoFetch() {
			SetAutoDownloadStatus( "Auto-fetching " + UserInfoPersister.KnownUsers.Count.ToString() + " users." );
			try {
				await VodList.AutoDownload( UserInfoPersister.KnownUsers.ToArray(), TwitchAPI, this );
			} catch ( Exception ex ) {
				SetAutoDownloadStatus( ex.ToString() );
				return;
			}

			System.Timers.Timer timer = new System.Timers.Timer( TimeSpan.FromHours( 7 ).TotalMilliseconds );
			SetAutoDownloadStatus( "Waiting for 7 hours to fetch again -- this will be roughly at " + ( DateTime.Now + TimeSpan.FromHours( 7 ) ).ToString() + "." );
			timer.Elapsed += delegate ( object sender, System.Timers.ElapsedEventArgs e ) {
				StartTimedAutoFetch();
			};
			timer.Enabled = true;
			timer.AutoReset = false;
			timer.Start();
		}

		public void SetAutoDownloadStatus( string s ) {
			if ( Util.AllowTimedAutoFetch ) {
				labelStatusBar.Text = "[" + DateTime.Now.ToString() + "] " + s;
			}
		}

		private void ObjectListViewDownloads_FormatRow( object sender, BrightIdeasSoftware.FormatRowEventArgs e ) {
			e.Item.Text = ( e.RowIndex + 1 ).ToString();
		}

		public static string ParseId( string url, out StreamService service ) {
			Uri uri = new Uri( url );

			if ( uri.Host.Contains( "twitch.tv" ) ) {
				service = StreamService.Twitch;
			} else if ( uri.Host.Contains( "hitbox.tv" ) ) {
				service = StreamService.Hitbox;
			} else if ( uri.Host.Contains( "youtube.com" ) ) {
				service = StreamService.Youtube;
			} else {
				throw new FormatException();
			}

			switch ( service ) {
				case StreamService.Twitch:
					return uri.Segments[uri.Segments.Length - 2].Trim( '/' ) + uri.Segments.Last();
				case StreamService.Hitbox:
					return uri.Segments.Last();
				case StreamService.Youtube:
					return Util.GetParameterFromUri( uri, "v" );
			}

			throw new FormatException();
		}

		public static StreamService GetServiceFromString( string s ) {
			switch ( s ) {
				case "Twitch":
					return StreamService.Twitch;
				case "Hitbox":
					return StreamService.Hitbox;
				case "Youtube":
					return StreamService.Youtube;
				default:
					throw new Exception( s + " is not a valid service." );
			}
		}

		private void buttonDownload_Click( object sender, EventArgs e ) {
			string unparsedId = textboxMediaId.Text.Trim();
			if ( unparsedId == "" ) { return; }

			StreamService service = StreamService.Unknown;
			string id;
			if ( unparsedId.Contains( '/' ) ) {
				try {
					id = ParseId( unparsedId, out service );
				} catch ( Exception ) {
					MessageBox.Show( "Can't determine service and/or video ID of " + unparsedId );
					return;
				}
			} else {
				id = unparsedId;
			}

			if ( comboBoxService.Text != "Autodetect" ) {
				service = GetServiceFromString( comboBoxService.Text );
			} else if ( service == StreamService.Unknown ) {
				MessageBox.Show( "Can't autodetect service without URL!" );
				return;
			}

			CreateAndEnqueueJob( service, id );
			textboxMediaId.Text = "";
			Task.Run( () => RunJob( service ) );
		}

		public struct CreateAndEnqueueJobReturnValue { public bool Success; public IVideoJob Job; }
		public CreateAndEnqueueJobReturnValue CreateAndEnqueueJob( StreamService service, string id, IVideoInfo info = null ) {
			IVideoJob job;
			switch ( service ) {
				case StreamService.Twitch:
					job = new TwitchVideoJob( TwitchAPI, id );
					break;
				case StreamService.TwitchChatReplay:
					job = new TwitchChatReplayJob( TwitchAPI, id );
					break;
				case StreamService.Hitbox:
					job = new HitboxVideoJob( id );
					break;
				case StreamService.Youtube:
					job = new YoutubeVideoJob( id );
					break;
				default:
					throw new Exception( service.ToString() + " isn't a known service." );
			}

			if ( info != null ) {
				job.VideoInfo = info;
			}

			bool success = EnqueueJob( job );

			return new CreateAndEnqueueJobReturnValue { Success = success, Job = job };
		}

		public CreateAndEnqueueJobReturnValue CreateAndEnqueueJob( IVideoInfo info ) {
			return CreateAndEnqueueJob( info.Service, info.VideoId, info );
		}

		public bool EnqueueJob( IVideoJob job ) {
			if ( JobSet.Contains( job ) ) {
				return false;
			}

			job.StatusUpdater = new StatusUpdate.ObjectListViewStatusUpdate( objectListViewDownloads, job );
			objectListViewDownloads.AddObject( job );
			JobSet.Add( job );
			job.Status = "Waiting...";
			lock ( JobQueueLock ) {
				WaitingJobs[job.VideoInfo.Service].Add( new WaitingVideoJob( job ) );
			}

			if ( Util.ShowToastNotifications ) {
				ToastUtil.ShowToast( "Enqueued " + job.HumanReadableJobName + "!" );
			}

			InvokeSaveJobs();

			return true;
		}

		public async Task RunJob( StreamService service, IVideoJob job = null, bool forceStart = false ) {
			bool runNewJob = false;
			lock ( JobQueueLock ) {
				// TODO: This almost certainly has odd results if one thread tries to dequeue a job that was force-started from somewhere else!
				if ( forceStart || JobsRunningPerType[service] < MaxJobsRunningPerType ) {
					if ( job != null ) {
						runNewJob = true;
						JobsRunningPerType[service] += 1;
					} else {
						if ( WaitingJobs[service].Count != 0 ) {
							WaitingVideoJob waitingJob = null;
							foreach ( WaitingVideoJob wj in WaitingJobs[service] ) {
								if ( wj.IsAllowedToStart() ) {
									waitingJob = wj;
									break;
								}
							}
							if ( waitingJob != null ) {
								job = waitingJob.Job;
								WaitingJobs[service].Remove( waitingJob );
								runNewJob = true;
								JobsRunningPerType[service] += 1;
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
						WaitingJobs[service].Add( new WaitingVideoJob( job, DateTime.UtcNow.AddMinutes( 10.0 ) ) );
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
						JobsRunningPerType[service] -= 1;
					}
				}

				InvokeSaveJobs();
				InvokePowerEvent();

				await RunJob( service );
			}
		}

		private void buttonSettings_Click( object sender, EventArgs e ) {
			new SettingsWindow().ShowDialog();
		}

		private void buttonFetchUser_Click( object sender, EventArgs e ) {
			new VodList( this, TwitchAPI ).Show();
		}

		private List<IVideoJob> LoadJobsInternal( System.IO.Stream fs ) {
			List<IVideoJob> jobs = new List<IVideoJob>();
			System.Runtime.Serialization.IFormatter formatter = new System.Runtime.Serialization.Formatters.Binary.BinaryFormatter();
			int jobCount = (int)formatter.Deserialize( fs );
			for ( int i = 0; i < jobCount; ++i ) {
				object obj = formatter.Deserialize( fs );
				IVideoJob job = obj as IVideoJob;
				if ( job != null ) {
					if ( job.JobStatus == VideoJobStatus.Running ) {
						job.JobStatus = VideoJobStatus.NotStarted;
						job.StatusUpdater = new StatusUpdate.NullStatusUpdate();
						job.Status = "Interrupted during: " + job.Status;
					}
					if ( job as TwitchVideoJob != null ) {
						( job as TwitchVideoJob ).TwitchAPI = TwitchAPI;
					} else if ( job as TwitchChatReplayJob != null ) {
						( job as TwitchChatReplayJob ).TwitchAPI = TwitchAPI;
					}
					job.StatusUpdater = new StatusUpdate.ObjectListViewStatusUpdate( objectListViewDownloads, job );
					if ( job.JobStatus != VideoJobStatus.Finished && job.JobStatus != VideoJobStatus.Dead ) {
						lock ( JobQueueLock ) {
							WaitingJobs[job.VideoInfo.Service].Add( new WaitingVideoJob( job ) );
						}
					}
					jobs.Add( job );
				}
			}
			return jobs;
		}

		private void LoadJobs() {
			lock ( Util.JobFileLock ) {
				List<IVideoJob> jobs = new List<IVideoJob>();

				try {
					using ( FileStream fs = System.IO.File.OpenRead( Util.VodBinaryPath ) ) {
						if ( fs.PeekUInt16() == 0x8B1F ) {
							using ( System.IO.Compression.GZipStream gzs = new System.IO.Compression.GZipStream( fs, System.IO.Compression.CompressionMode.Decompress ) ) {
								jobs = LoadJobsInternal( gzs );
							}
						} else {
							jobs = LoadJobsInternal( fs );
						}
					}
				} catch ( System.Runtime.Serialization.SerializationException ) { } catch ( FileNotFoundException ) { }

				objectListViewDownloads.AddObjects( jobs );
				foreach ( IVideoJob job in jobs ) {
					JobSet.Add( job );
				}
				for ( int i = 0; i < objectListViewDownloads.Items.Count; ++i ) {
					objectListViewDownloads.Items[i].Text = ( i + 1 ).ToString();
					if ( i % 2 == 1 ) {
						objectListViewDownloads.Items[i].BackColor = objectListViewDownloads.AlternateRowBackColorOrDefault;
					}
				}
			}

			foreach ( StreamService s in Enum.GetValues( typeof( StreamService ) ) ) {
				Task.Run( () => RunJob( s ) );
			}
		}

		private System.Timers.Timer SaveJobTimer = null;
		private static object SaveJobTimerLock = new object();
		private void InvokeSaveJobs() {
			lock ( SaveJobTimerLock ) {
				if ( SaveJobTimer == null ) {
					SaveJobTimer = new System.Timers.Timer( TimeSpan.FromMinutes( 1 ).TotalMilliseconds );
					SaveJobTimer.Elapsed += delegate ( object sender, System.Timers.ElapsedEventArgs e ) {
						SaveJobs();
					};
					SaveJobTimer.AutoReset = false;
				}
				SaveJobTimer.Stop();
				SaveJobTimer.Start();
			}
		}

		private void SaveJobsInternal( System.IO.Stream fs, List<IVideoJob> jobs ) {
			System.Runtime.Serialization.IFormatter formatter = new System.Runtime.Serialization.Formatters.Binary.BinaryFormatter();
			formatter.Serialize( fs, jobs.Count );
			foreach ( var job in jobs ) {
				formatter.Serialize( fs, job );
			}
		}

		private void SaveJobs() {
			Invoke( (MethodInvoker)( () => { 
			lock ( SaveJobTimerLock ) {
				if ( SaveJobTimer != null ) {
					SaveJobTimer.Stop();
				}
			}
			lock ( Util.JobFileLock ) {
				if ( Util.ShowToastNotifications ) {
					ToastUtil.ShowToast( DateTime.Now + ": Saving jobs." );
				}

				List<IVideoJob> jobs = new List<IVideoJob>();
				foreach ( var item in objectListViewDownloads.Objects ) {
					IVideoJob job = item as IVideoJob;
					if ( job != null ) {
						jobs.Add( job );
					}
				}

				using ( FileStream fs = System.IO.File.Create( Util.VodBinaryTempPath ) )
				using ( System.IO.Compression.GZipStream gzs = new System.IO.Compression.GZipStream( fs, System.IO.Compression.CompressionLevel.Optimal ) ) {
					SaveJobsInternal( gzs, jobs );
				}
				if ( System.IO.File.Exists( Util.VodBinaryPath ) ) {
					Thread.Sleep( 100 );
					System.IO.File.Delete( Util.VodBinaryPath );
					Thread.Sleep( 100 );
				}
				System.IO.File.Move( Util.VodBinaryTempPath, Util.VodBinaryPath );
				Thread.Sleep( 100 );
			}
			} ) );
		}

		private void DownloadForm_FormClosing( object sender, FormClosingEventArgs e ) {
			SaveJobs();
		}

		private void objectListViewDownloads_ButtonClick( object sender, BrightIdeasSoftware.CellClickEventArgs e ) {
			switch ( e.SubItem.Text ) {
				case "Remove":
					objectListViewDownloads.RemoveObject( e.Model );
					JobSet.Remove( (IVideoJob)e.Model );
					// TODO: Remove from job queue too!
					// TODO: Stop job when removed & running!
					break;
				case "Force Start":
					IVideoJob job = (IVideoJob)e.Model;
					Task.Run( () => RunJob( job.VideoInfo.Service, job, true ) );
					break;
			}
		}

		private void objectListViewDownloads_CellEditStarting( object sender, BrightIdeasSoftware.CellEditEventArgs e ) {
			return;
		}

		private void InvokePowerEvent() {
			lock ( JobQueueLock ) {
				foreach ( var kvp in WaitingJobs ) {
					if ( kvp.Value.Count > 0 ) {
						return;
					}
				}
				foreach ( var kvp in JobsRunningPerType ) {
					if ( kvp.Value > 0 ) {
						return;
					}
				}
			}
			PowerEvent();
		}

		private void PowerEvent() {
			switch ( SelectedPowerStateOption ) {
				case "Close VodArchiver":
					Invoke( (MethodInvoker)( () => { Close(); } ) );
					break;
				case "Sleep":
					SaveJobs();
					Application.SetSuspendState( PowerState.Suspend, false, false );
					break;
				case "Hibernate":
					SaveJobs();
					Application.SetSuspendState( PowerState.Hibernate, false, false );
					break;
			}
		}

		private string SelectedPowerStateOption = "";
		private void comboBoxPowerStateWhenDone_SelectedIndexChanged( object sender, EventArgs e ) {
			SelectedPowerStateOption = comboBoxPowerStateWhenDone.Text;
		}
	}
}
