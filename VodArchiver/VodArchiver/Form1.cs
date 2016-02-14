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
	public partial class Form1 : Form {
		Twixel TwitchAPI;
		System.Collections.Concurrent.ConcurrentQueue<IVideoJob> JobQueue;
		private int RunningJobs;
		private object Lock = new object();

		public Form1() {
			InitializeComponent();
			comboBoxService.SelectedIndex = 0;
			TwitchAPI = new Twixel( "", "", Twixel.APIVersion.v3 );
			JobQueue = new System.Collections.Concurrent.ConcurrentQueue<IVideoJob>();
			RunningJobs = 0;
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

		private async void buttonDownload_Click( object sender, EventArgs e ) {
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
			await RunJob();
		}

		public void CreateAndEnqueueJob( StreamService service, string id ) {
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

		public async Task RunJob() {
			bool runNewJob = false;
			IVideoJob job = null;
			lock ( Lock ) {
				if ( RunningJobs < 3 && JobQueue.TryDequeue( out job ) ) {
					++RunningJobs;
					runNewJob = true;
				}
			}

			if ( runNewJob ) {
				try {
					await job.Run();
				} catch ( Exception ex ) {
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
	}
}
