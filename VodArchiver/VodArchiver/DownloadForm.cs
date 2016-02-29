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
		const int MaxRunningJobs = 3;

		public DownloadForm() {
			InitializeComponent();
			comboBoxService.SelectedIndex = 0;
			TwitchAPI = new Twixel( "", "", Twixel.APIVersion.v3 );
			JobQueue = new System.Collections.Concurrent.ConcurrentQueue<IVideoJob>();
			RunningJobs = 0;

			objectListViewDownloads.SecondarySortColumn = objectListViewDownloads.GetColumn( "Video ID" );
			objectListViewDownloads.SecondarySortOrder = SortOrder.Ascending;

			LoadJobs();
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

		public void CreateAndEnqueueJob( StreamService service, string id ) {
			if ( JobExists( service, id ) ) {
				return;
			}

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

			job.StatusUpdater = new StatusUpdate.ObjectListViewStatusUpdate( objectListViewDownloads, job );
			objectListViewDownloads.AddObject( job );
			job.Status = "Waiting...";
			JobQueue.Enqueue( job );
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
					}
				} catch ( Exception ex ) {
					job.JobStatus = VideoJobStatus.NotStarted;
					job.Status = "ERROR: " + ex.ToString();
				}

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
			try {
				objectListViewDownloads.BeginUpdate();
				using ( FileStream fs = System.IO.File.OpenRead( Path.Combine( Application.LocalUserAppDataPath, "vods.bin" ) ) ) {
					System.Runtime.Serialization.IFormatter formatter = new System.Runtime.Serialization.Formatters.Binary.BinaryFormatter();
					int jobCount = (int)formatter.Deserialize( fs );
					for ( int i = 0; i < jobCount; ++i ) {
						object obj = formatter.Deserialize( fs );
						IVideoJob job = obj as IVideoJob;
						if ( job != null ) {
							if ( job as TwitchVideoJob != null ) {
								( job as TwitchVideoJob ).TwitchAPI = TwitchAPI;
							}
							job.StatusUpdater = new StatusUpdate.ObjectListViewStatusUpdate( objectListViewDownloads, job );
							objectListViewDownloads.AddObject( job );
							if ( job.JobStatus != VideoJobStatus.Finished ) {
								JobQueue.Enqueue( job );
							}
						}
					}
					fs.Close();
				}
			} catch ( System.Runtime.Serialization.SerializationException ) { } catch ( FileNotFoundException ) { } finally {
				objectListViewDownloads.EndUpdate();
			}

			for ( int i = 0; i < MaxRunningJobs; ++i ) {
				Task.Run( () => RunJob() );
			}
		}

		private void SaveJobs() {
			using ( FileStream fs = System.IO.File.Create( Path.Combine( Application.LocalUserAppDataPath, "vods.tmp" ) ) ) {
				System.Runtime.Serialization.IFormatter formatter = new System.Runtime.Serialization.Formatters.Binary.BinaryFormatter();
				formatter.Serialize( fs, objectListViewDownloads.Items.Count );
				foreach ( var item in objectListViewDownloads.Items ) {
					IVideoJob job = ( item as BrightIdeasSoftware.OLVListItem )?.RowObject as IVideoJob;
					formatter.Serialize( fs, job );
				}
				fs.Close();
			}
			if ( System.IO.File.Exists( Path.Combine( Application.LocalUserAppDataPath, "vods.bin" ) ) ) {
				System.IO.File.Delete( Path.Combine( Application.LocalUserAppDataPath, "vods.bin" ) );
			}
			System.IO.File.Move( Path.Combine( Application.LocalUserAppDataPath, "vods.tmp" ), Path.Combine( Application.LocalUserAppDataPath, "vods.bin" ) );
		}

		private void DownloadForm_FormClosing( object sender, FormClosingEventArgs e ) {
			SaveJobs();
		}

		private void objectListViewDownloads_ButtonClick( object sender, BrightIdeasSoftware.CellClickEventArgs e ) {
			switch ( e.SubItem.Text ) {
				case "Force Start":
					IVideoJob job = (IVideoJob)e.Model;
					Task.Run( () => RunJob( job, true ) );
					break;
			}
		}
	}
}
