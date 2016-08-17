using System;
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
	public partial class DownloadForm : Form {
		Twixel TwitchAPI;
		System.Collections.Concurrent.ConcurrentQueue<IVideoJob> JobQueue;
		private int RunningJobs;
		private object Lock = new object();
		public const int MaxRunningJobs = 3;

		Task TimedAutoFetchTask;

		public DownloadForm() {
			InitializeComponent();
			objectListViewDownloads.CellEditActivation = BrightIdeasSoftware.ObjectListView.CellEditActivateMode.DoubleClick;

			comboBoxService.SelectedIndex = 0;
			TwitchAPI = new Twixel( "", "", Twixel.APIVersion.v3 );
			JobQueue = new System.Collections.Concurrent.ConcurrentQueue<IVideoJob>();
			RunningJobs = 0;

			if ( !Util.AllowTimedAutoFetch ) {
				labelStatusBar.Hide();
				objectListViewDownloads.Size = new Size( objectListViewDownloads.Size.Width, objectListViewDownloads.Size.Height + 16 );
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
			} else {
				throw new FormatException();
			}

			switch ( service ) {
				case StreamService.Twitch:
					return uri.Segments[uri.Segments.Length - 2].Trim( '/' ) + uri.Segments.Last();
				case StreamService.Hitbox:
					return uri.Segments.Last();
			}

			throw new FormatException();
		}

		public static StreamService GetServiceFromString( string s ) {
			switch ( s ) {
				case "Twitch":
					return StreamService.Twitch;
				case "Hitbox":
					return StreamService.Hitbox;
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
			Task.Run( () => RunJob() );
		}

		public bool JobExists( StreamService service, string id ) {
			foreach ( var item in objectListViewDownloads.Items ) {
				IVideoJob job = ( item as BrightIdeasSoftware.OLVListItem )?.RowObject as IVideoJob;
				if ( job != null && job.VideoInfo.Service == service && job.VideoInfo.VideoId == id ) {
					return true;
				}
			}
			return false;
		}

		public bool CreateAndEnqueueJob( StreamService service, string id, IVideoInfo info = null ) {
			IVideoJob job;
			switch ( service ) {
				case StreamService.Twitch:
					job = new TwitchVideoJob( TwitchAPI, id );
					break;
				case StreamService.Hitbox:
					job = new HitboxVideoJob( id );
					break;
				default:
					throw new Exception( service.ToString() + " isn't a known service." );
			}

			if ( info != null ) {
				job.VideoInfo = info;
			}

			return EnqueueJob( job );
		}

		public bool CreateAndEnqueueJob( IVideoInfo info ) {
			return CreateAndEnqueueJob( info.Service, info.VideoId, info );
		}

		public bool EnqueueJob( IVideoJob job ) {
			if ( JobExists( job.VideoInfo.Service, job.VideoInfo.VideoId ) ) {
				return false;
			}

			job.StatusUpdater = new StatusUpdate.ObjectListViewStatusUpdate( objectListViewDownloads, job );
			objectListViewDownloads.AddObject( job );
			job.Status = "Waiting...";
			JobQueue.Enqueue( job );

			if ( Util.ShowToastNotifications ) {
				ToastUtil.ShowToast( "Enqueued " + job.HumanReadableJobName + "!" );
			}

			SaveJobs();

			return true;
		}

		public async Task RunJob( IVideoJob job = null, bool forceStart = false ) {
			bool runNewJob = false;
			lock ( Lock ) {
				if ( ( forceStart || RunningJobs < MaxRunningJobs ) && ( job != null || JobQueue.TryDequeue( out job ) ) ) {
					// TODO: This almost certainly has odd results if one thread tries to dequeue a job that was force-started from somewhere else!
					++RunningJobs;
					runNewJob = true;
				}
			}

			if ( runNewJob ) {
				try {
					if ( job.JobStatus != VideoJobStatus.Finished ) {
						await job.Run();
						if ( Util.ShowToastNotifications ) {
							ToastUtil.ShowToast( "Downloaded " + job.HumanReadableJobName + "!" );
						}
					}
				} catch ( Exception ex ) {
					job.JobStatus = VideoJobStatus.NotStarted;
					job.Status = "ERROR: " + ex.ToString();
					if ( Util.ShowToastNotifications ) {
						ToastUtil.ShowToast( "Failed to download " + job.HumanReadableJobName + ": " + ex.ToString() );
					}
				}

				SaveJobs();

				lock ( Lock ) {
					--RunningJobs;
				}
				await RunJob();
			}
		}

		private void buttonSettings_Click( object sender, EventArgs e ) {
			new SettingsWindow().ShowDialog();
		}

		private void buttonFetchUser_Click( object sender, EventArgs e ) {
			new VodList( this, TwitchAPI ).Show();
		}

		private void LoadJobs() {
			lock ( Util.JobFileLock ) {
				List<IVideoJob> jobs = new List<IVideoJob>();

				try {
					using ( FileStream fs = System.IO.File.OpenRead( Util.VodBinaryPath ) ) {
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
								}
								job.StatusUpdater = new StatusUpdate.ObjectListViewStatusUpdate( objectListViewDownloads, job );
								if ( job.JobStatus != VideoJobStatus.Finished ) {
									JobQueue.Enqueue( job );
								}
								jobs.Add( job );
							}
						}
						fs.Close();
					}
				} catch ( System.Runtime.Serialization.SerializationException ) { } catch ( FileNotFoundException ) { }

				objectListViewDownloads.AddObjects( jobs );
				for ( int i = 0; i < objectListViewDownloads.Items.Count; ++i ) {
					objectListViewDownloads.Items[i].Text = ( i + 1 ).ToString();
					if ( i % 2 == 1 ) {
						objectListViewDownloads.Items[i].BackColor = objectListViewDownloads.AlternateRowBackColorOrDefault;
					}
				}
			}

			for ( int i = 0; i < MaxRunningJobs; ++i ) {
				Task.Run( () => RunJob() );
			}
		}

		private void SaveJobs() {
			lock ( Util.JobFileLock ) {
				using ( FileStream fs = System.IO.File.Create( Util.VodBinaryTempPath ) ) {
					System.Runtime.Serialization.IFormatter formatter = new System.Runtime.Serialization.Formatters.Binary.BinaryFormatter();
					formatter.Serialize( fs, objectListViewDownloads.Items.Count );
					foreach ( var item in objectListViewDownloads.Items ) {
						IVideoJob job = ( item as BrightIdeasSoftware.OLVListItem )?.RowObject as IVideoJob;
						formatter.Serialize( fs, job );
					}
					fs.Close();
				}
				if ( System.IO.File.Exists( Util.VodBinaryPath ) ) {
					System.IO.File.Delete( Util.VodBinaryPath );
				}
				System.IO.File.Move( Util.VodBinaryTempPath, Util.VodBinaryPath );
			}
		}

		private void DownloadForm_FormClosing( object sender, FormClosingEventArgs e ) {
			SaveJobs();
		}

		private void objectListViewDownloads_ButtonClick( object sender, BrightIdeasSoftware.CellClickEventArgs e ) {
			switch ( e.SubItem.Text ) {
				case "Remove":
					objectListViewDownloads.RemoveObject( e.Model );
					// TODO: Remove from job queue too!
					// TODO: Stop job when removed & running!
					break;
				case "Force Start":
					IVideoJob job = (IVideoJob)e.Model;
					Task.Run( () => RunJob( job, true ) );
					break;
			}
		}

		private void objectListViewDownloads_CellEditStarting( object sender, BrightIdeasSoftware.CellEditEventArgs e ) {
			return;
		}
	}
}
