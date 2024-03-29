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
using System.Xml;
using VodArchiver.Tasks;
using VodArchiver.UserInfo;
using VodArchiver.VideoInfo;
using VodArchiver.VideoJobs;

namespace VodArchiver {
	public partial class DownloadForm : Form {
		HashSet<IVideoJob> JobSet;
		long IndexCounter = 0;

		Dictionary<StreamService, VideoTaskGroup> VideoTaskGroups;
		Dictionary<ServiceVideoCategoryType, FetchTaskGroup> FetchTaskGroups;
		CancellationTokenSource CancellationTokenSource;
		ObjectListViewUpdaterTask ObjectListViewUpdater;

		List<string> StatusMessages = new List<string>();
		object StatusMessageLock = new object();
		Task StatusMessageTask = null;

		public DownloadForm() {
			InitializeComponent();
			ObjectListViewUpdater = new ObjectListViewUpdaterTask( objectListViewDownloads );
			CancellationTokenSource = new CancellationTokenSource();
			objectListViewDownloads.CellEditActivation = BrightIdeasSoftware.ObjectListView.CellEditActivateMode.DoubleClick;

			comboBoxService.SelectedIndex = 0;
			comboBoxPowerStateWhenDone.SelectedIndex = 0;

			JobSet = new HashSet<IVideoJob>();

			VideoTaskGroups = new Dictionary<StreamService, VideoTaskGroup>();
			foreach ( StreamService s in Enum.GetValues( typeof( StreamService ) ) ) {
				VideoTaskGroups.Add( s, new VideoTaskGroup( s, () => InvokeSaveJobs(), () => InvokePowerEvent(), CancellationTokenSource.Token ) );
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

			objectListViewDownloads.FilterMenuBuildStrategy.MaxObjectsToConsider = int.MaxValue;

			objectListViewDownloads.FormatRow += ObjectListViewDownloads_FormatRow;

			foreach ( BrightIdeasSoftware.OLVColumn col in objectListViewDownloads.Columns ) {
				if ( col.Text == "Status" ) {
					System.Collections.ArrayList statusFilters = new System.Collections.ArrayList();
					statusFilters.Add( VideoJobs.VideoJobStatus.NotStarted );
					statusFilters.Add( VideoJobs.VideoJobStatus.Running );
					col.ValuesChosenForFiltering = statusFilters;
					objectListViewDownloads.ModelFilter = objectListViewDownloads.CreateColumnFilter();
					objectListViewDownloads.UseFiltering = true;
					break;
				}
			}

			LoadJobs();

			if ( Util.AllowTimedAutoFetch ) {
				StartTimedAutoFetch();
			}
		}

		private void StartTimedAutoFetch() {
			if ( Util.AllowTimedAutoFetch ) {
				FetchTaskGroups = new Dictionary<ServiceVideoCategoryType, FetchTaskGroup>();
				foreach ( List<ServiceVideoCategoryType> types in ServiceVideoCategoryGroups.Groups ) {
					FetchTaskGroup ftg = new FetchTaskGroup(this, CancellationTokenSource.Token);
					foreach ( ServiceVideoCategoryType svc in types ) {
						FetchTaskGroups.Add( svc, ftg );
						foreach ( IUserInfo ui in UserInfoPersister.GetKnownUsers() ) {
							if ( ui.AutoDownload && ui.Type == svc ) {
								ftg.Add( ui );
							}
						}
					}
				}

				StatusMessageTask = StatusMessageThreadFunc(CancellationTokenSource.Token);
			}
		}

		public void AddStatusMessage(string s) {
			if (Util.AllowTimedAutoFetch) {
				string message = "[" + DateTime.Now.ToString() + "] " + s;
				lock (StatusMessageLock) {
					if (StatusMessages.Count > 10000) {
						StatusMessages.RemoveRange(0, 2000);
					}

					StatusMessages.Add(message);
				}
			}
		}

		private async Task StatusMessageThreadFunc(CancellationToken cancellationToken) {
			while (true) {
				try {
					if (cancellationToken.IsCancellationRequested) {
						return;
					}

					await Task.Delay(500, cancellationToken);

					string msg = GetLatestStatusMessageInternal();
					Invoke((MethodInvoker)(() => {
						if (labelStatusBar.Text != msg) {
							labelStatusBar.Text = msg;
						}
					}));
				} catch (Exception ex) {
					continue;
				}
			}
		}

		private string GetLatestStatusMessageInternal() {
			lock (StatusMessageLock) {
				if (StatusMessages.Count > 0) {
					return StatusMessages[StatusMessages.Count - 1];
				} else {
					return "";
				}
			}
		}

		private void ObjectListViewDownloads_FormatRow( object sender, BrightIdeasSoftware.FormatRowEventArgs e ) {
		}

		public static string ParseId( string url, out StreamService service ) {
			Uri uri = new Uri( url );

			if ( uri.Host.Contains( "twitch.tv" ) ) {
				service = StreamService.Twitch;
			} else if ( uri.Host.Contains( "hitbox.tv" ) || uri.Host.Contains( "smashcast.tv" ) ) {
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

			if (service == StreamService.Twitch) {
				CreateAndEnqueueJob(StreamService.Twitch, id);
				CreateAndEnqueueJob(StreamService.TwitchChatReplay, id);
			} else {
				CreateAndEnqueueJob(service, id);
			}
			textboxMediaId.Text = "";
		}

		public struct CreateAndEnqueueJobReturnValue { public bool Success; public IVideoJob Job; }
		public CreateAndEnqueueJobReturnValue CreateAndEnqueueJob( StreamService service, string id, IVideoInfo info = null ) {
			IVideoJob job;
			switch ( service ) {
				case StreamService.Twitch:
					job = new TwitchVideoJob( id );
					break;
				case StreamService.TwitchChatReplay:
					job = new TwitchChatReplayJob( id );
					break;
				case StreamService.Hitbox:
					job = new HitboxVideoJob( id );
					break;
				case StreamService.Youtube:
					job = new YoutubeVideoJob( id );
					break;
				case StreamService.RawUrl:
					job = new GenericFileJob( id );
					break;
				case StreamService.FFMpegJob:
					job = new FFMpegReencodeJob( id );
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

			job.StatusUpdater = new StatusUpdate.ObjectListViewStatusUpdate( ObjectListViewUpdater, job );
			job.Index = ++IndexCounter;
			objectListViewDownloads.AddObject( job );
			JobSet.Add( job );
			job.Status = "Waiting...";
			if (VideoTaskGroups[job.VideoInfo.Service].AutoEnqueue) {
				VideoTaskGroups[job.VideoInfo.Service].Add( new WaitingVideoJob( job ) );
			}

			InvokeSaveJobs();

			return true;
		}

		private void buttonSettings_Click( object sender, EventArgs e ) {
			new SettingsWindow().ShowDialog();
		}

		private void buttonSettings_MouseUp( object sender, MouseEventArgs e ) {
			if ( e.Button == MouseButtons.Right ) {
				List<IVideoJob> jobs = new List<IVideoJob>();
				lock ( Util.JobFileLock ) {
					foreach ( var item in objectListViewDownloads.Objects ) {
						IVideoJob job = item as IVideoJob;
						if ( job != null ) {
							jobs.Add( job );
						}
					}
				}

				new AutoRenameWindow( jobs ).ShowDialog();
			}
		}

		private void buttonFetchUser_Click( object sender, EventArgs e ) {
			new VodList( this ).Show();
		}

		private void buttonFetchUser_MouseUp( object sender, MouseEventArgs e ) {
			if ( e.Button == MouseButtons.Right ) {
				new VideoSplitWindow( this ).Show();
			}
		}

		private void LoadJobs() {
			lock ( Util.JobFileLock ) {
				List<IVideoJob> jobs = new List<IVideoJob>();

				try {
					XmlDocument doc = new XmlDocument();
					using ( FileStream fs = System.IO.File.OpenRead( Util.VodXmlPath ) ) {
						if ( fs.PeekUInt16() == 0x8B1F ) {
							using ( System.IO.Compression.GZipStream gzs = new System.IO.Compression.GZipStream( fs, System.IO.Compression.CompressionMode.Decompress ) ) {
								doc.Load( gzs );
							}
						} else {
							doc.Load( fs );
						}
					}
					foreach ( XmlNode node in doc.SelectNodes( "//root/Job" ) ) {
						IVideoJob job = IVideoJob.Deserialize( node );
						if ( job != null ) {
							if ( job.JobStatus == VideoJobStatus.Running ) {
								job.JobStatus = VideoJobStatus.NotStarted;
								job.StatusUpdater = new StatusUpdate.NullStatusUpdate();
								job.Status = "Interrupted during: " + job.Status;
							}
							job.StatusUpdater = new StatusUpdate.ObjectListViewStatusUpdate( ObjectListViewUpdater, job );
							jobs.Add( job );
						}
					}
				} catch ( System.Runtime.Serialization.SerializationException ) { } catch ( FileNotFoundException ) { }

				foreach ( IVideoJob job in jobs ) {
					job.Index = ++IndexCounter;
				}
				objectListViewDownloads.AddObjects( jobs );
				foreach ( IVideoJob job in jobs ) {
					JobSet.Add( job );
				}
				for ( int i = 0; i < objectListViewDownloads.Items.Count; ++i ) {
					if ( i % 2 == 1 ) {
						objectListViewDownloads.Items[i].BackColor = objectListViewDownloads.AlternateRowBackColorOrDefault;
					}
				}
			}
		}

		private System.Timers.Timer SaveJobTimer = null;
		private static object SaveJobTimerLock = new object();
		public void InvokeSaveJobs() {
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
			XmlDocument document = new XmlDocument();
			XmlNode node = document.CreateElement( "root" );
			foreach ( var job in jobs ) {
				node.AppendChild( job.Serialize( document, document.CreateElement( "Job" ) ) );
			}
			document.AppendChild( node );
			using ( StreamWriter sw = new StreamWriter( fs, new UTF8Encoding( false, true ), 1024 * 1024, true ) ) {
				document.Save( XmlWriter.Create( sw ) );
			}
		}

		private void SaveJobs() {
			lock ( Util.JobFileLock ) {
				using ( MemoryStream memoryStream = new MemoryStream() ) {
					Invoke( (MethodInvoker)( () => {
						lock ( SaveJobTimerLock ) {
							if ( SaveJobTimer != null ) {
								SaveJobTimer.Stop();
							}
						}

						List<IVideoJob> jobs = new List<IVideoJob>();
						foreach ( var item in objectListViewDownloads.Objects ) {
							IVideoJob job = item as IVideoJob;
							if ( job != null ) {
								jobs.Add( job );
							}
						}

						SaveJobsInternal( memoryStream, jobs );
					} ) );

					long count = memoryStream.Position;
					memoryStream.Position = 0;
					using ( FileStream fs = System.IO.File.Create( Util.VodXmlTempPath ) )
					using ( System.IO.Compression.GZipStream gzs = new System.IO.Compression.GZipStream( fs, System.IO.Compression.CompressionLevel.Optimal ) ) {
						Util.CopyStream( memoryStream, gzs, count );
					}
					if ( System.IO.File.Exists( Util.VodXmlPath ) ) {
						Thread.Sleep( 100 );
						System.IO.File.Delete( Util.VodXmlPath );
						Thread.Sleep( 100 );
					}
					System.IO.File.Move( Util.VodXmlTempPath, Util.VodXmlPath );
					Thread.Sleep( 100 );
				}
			}
		}

		private async Task WaitForAllTasksToEnd() {
			// TODO: wait for fetches to end too
			if (StatusMessageTask != null) {
				await StatusMessageTask;
			}
			foreach ( var group in VideoTaskGroups ) {
				await group.Value.WaitForJobRunnerThreadToEnd();
			}
			foreach (var group in FetchTaskGroups) {
				await group.Value.WaitForFetchThreadToEnd();
			}
		}

		private bool closingAlready = false;
		private async void DownloadForm_FormClosing( object sender, FormClosingEventArgs e ) {
			// workaround; without this, the form can close before the SaveJobs() executes because, as I understand it, the await gives control back to the callee, which procedes to shut down the application
			if ( !closingAlready ) {
				closingAlready = true;
				Enabled = false;
				e.Cancel = true;
				CancellationTokenSource.Cancel();
				await WaitForAllTasksToEnd();
				SaveJobs();
				Close();
			}
		}

		private void objectListViewDownloads_CellEditStarting( object sender, BrightIdeasSoftware.CellEditEventArgs e ) {
			return;
		}

		public void InvokePowerEvent() {
			foreach ( var kvp in VideoTaskGroups ) {
				if ( !kvp.Value.IsEmpty() ) {
					return;
				}
			}
			PowerEvent();
			comboBoxPowerStateWhenDone.SelectedIndex = 0;
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

		private void objectListViewDownloads_CellRightClick( object sender, BrightIdeasSoftware.CellRightClickEventArgs e ) {
			e.MenuStrip = this.CreateItemMenu( e.Model, e.Column );
		}

		private ContextMenuStrip CreateItemMenu( object model, BrightIdeasSoftware.OLVColumn column ) {
			if ( model != null && model as IVideoJob != null ) {
				IVideoJob job = model as IVideoJob;
				ContextMenuStrip menu = new ContextMenuStrip();

				{
					IUserInputRequest uir = job.UserInputRequest;
					if ( uir != null ) {
						ToolStripMenuItem item = new ToolStripMenuItem( uir.GetQuestion() );
						foreach ( string option in uir.GetOptions() ) {
							item.DropDownItems.Add( option ).Click+= ( sender, e ) => {
								if ( uir != null ) {
									uir.SelectOption( option );
								}
							};
						}
						menu.Items.Add( item );
						menu.Items.Add( new ToolStripSeparator() );
					}
				}

				bool queueOptionsAvailable = false;
				if ( ( job.JobStatus == VideoJobStatus.NotStarted || job.JobStatus == VideoJobStatus.Dead ) && !VideoTaskGroups[job.VideoInfo.Service].IsInQueue( job ) ) {
					queueOptionsAvailable = true;
					ToolStripItem item = menu.Items.Add( "Enqueue" );
					item.Click += ( sender, e ) => {
						VideoTaskGroups[job.VideoInfo.Service].Add( new WaitingVideoJob( job ) );
					};
				}

				if ( ( job.JobStatus == VideoJobStatus.NotStarted || job.JobStatus == VideoJobStatus.Dead ) && VideoTaskGroups[job.VideoInfo.Service].IsInQueue( job ) ) {
					queueOptionsAvailable = true;
					ToolStripItem item = menu.Items.Add( "Dequeue" );
					item.Click += ( sender, e ) => {
						VideoTaskGroups[job.VideoInfo.Service].Dequeue( job );
					};
				}

				if ( job.JobStatus == VideoJobStatus.NotStarted || job.JobStatus == VideoJobStatus.Dead ) {
					queueOptionsAvailable = true;
					ToolStripItem item = menu.Items.Add( "Download now" );
					item.Click += ( sender, e ) => {
						if ( job.JobStatus == VideoJobStatus.NotStarted || job.JobStatus == VideoJobStatus.Dead ) {
							VideoTaskGroups[job.VideoInfo.Service].Add( new WaitingVideoJob( job, true ) );
						}
					};
				}

				if ( queueOptionsAvailable ) {
					menu.Items.Add( new ToolStripSeparator() );
				}


				{
					ToolStripItem item = menu.Items.Add("Copy Video ID");
					item.Click += (sender, e) => {
						string id = job?.VideoInfo?.VideoId;
						if (id != null) {
							Clipboard.SetText(id);
						}
					};
				}
				{
					ToolStripItem item = menu.Items.Add("Copy Output Filename");
					item.Click += (sender, e) => {
						string fn = job?.GenerateOutputFilename();
						if (fn != null) {
							Clipboard.SetText(fn);
						}
					};
				}
				{
					ToolStripItem item = menu.Items.Add("Copy Status");
					item.Click += (sender, e) => {
						string fn = job?.Status;
						if (fn != null) {
							Clipboard.SetText(fn);
						}
					};
				}

				menu.Items.Add(new ToolStripSeparator());

				if ( job.JobStatus == VideoJobStatus.Running ) {
					ToolStripItem item = menu.Items.Add( "Stop" );
					item.Click += ( sender, e ) => {
						if ( job.JobStatus == VideoJobStatus.Running ) {
							VideoTaskGroups[job.VideoInfo.Service].CancelJob( job );
						}
					};
				}

				if ( job.JobStatus == VideoJobStatus.NotStarted ) {
					ToolStripItem item = menu.Items.Add( "Kill" );
					item.Click += ( sender, e ) => {
						if ( job.JobStatus == VideoJobStatus.NotStarted ) {
							job.JobStatus = VideoJobStatus.Dead;
							job.Status = "[Manually killed] " + job.Status;
						}
					};
				}

				if ( job.JobStatus != VideoJobStatus.Running ) {
					ToolStripItem item = menu.Items.Add( "Remove" );
					item.Click += ( sender, e ) => {
						if ( job.JobStatus != VideoJobStatus.Running ) {
							objectListViewDownloads.RemoveObject( job );
							JobSet.Remove( job );
						}
					};
				}

				return menu.Items.Count > 0 ? menu : null;
			}
			return null;
		}

		private async void SanityCheckYoutube() {
			List<string> paths = new List<string>();
			foreach ( string path in paths ) {
				foreach ( string file in Directory.EnumerateFiles( path, "youtube_*.mkv", SearchOption.AllDirectories ) ) {
					try {
						string filepart = Path.GetFileNameWithoutExtension( file );
						string id = filepart.Substring( filepart.Length - 11, 11 );

						List<IVideoJob> jobs = new List<IVideoJob>();
						foreach ( var item in objectListViewDownloads.Objects ) {
							IVideoJob job = item as IVideoJob;
							if ( job != null ) {
								if ( job.VideoInfo.Service == StreamService.Youtube && job.VideoInfo.VideoId == id && job.JobStatus == VideoJobStatus.Finished ) {
									try {
										// sanity check
										TimeSpan actualVideoLength = ( await FFMpegUtil.Probe( file ) ).Duration;
										TimeSpan expectedVideoLength = job.VideoInfo.VideoLength;
										if ( actualVideoLength.Subtract( expectedVideoLength ).Duration() > TimeSpan.FromSeconds( 5 ) ) {
											// if difference is bigger than 5 seconds something is off, report
											job.Status = "Large time difference between expected (" + expectedVideoLength.ToString() + ") and actual (" + actualVideoLength.ToString() + "), reenqueueing. (" + file + ")";
											job.JobStatus = VideoJobStatus.NotStarted;
										} else {
											job.Status = "Sane.";
										}
									} catch ( Exception ex ) {
										job.Status = "Exception while sanity checking: " + ex.ToString();
										job.JobStatus = VideoJobStatus.NotStarted;
									}
								}
							}
						}

					} catch ( Exception ) {
					}
				}
			}
		}

		private async void SanityCheckTwitch() {
			List<string> paths = new List<string>();
			foreach ( string path in paths ) {
				foreach ( string file in Directory.EnumerateFiles( path, "twitch_*.mp4", SearchOption.AllDirectories ) ) {
					try {
						string filepart = Path.GetFileNameWithoutExtension( file );
						long tmp;
						string id = filepart.Split( '_' ).Where( x => x.StartsWith( "v" ) && long.TryParse( x.Substring( 1 ), out tmp ) ).First();

						List<IVideoJob> jobs = new List<IVideoJob>();
						foreach ( var item in objectListViewDownloads.Objects ) {
							IVideoJob job = item as IVideoJob;
							if ( job != null ) {
								if ( job.VideoInfo.Service == StreamService.Twitch && job.VideoInfo.VideoId == id && job.JobStatus == VideoJobStatus.Finished ) {
									try {
										// sanity check
										TimeSpan actualVideoLength = ( await FFMpegUtil.Probe( file ) ).Duration;
										TimeSpan expectedVideoLength = job.VideoInfo.VideoLength;
										if ( actualVideoLength.Subtract( expectedVideoLength ).Duration() > TimeSpan.FromSeconds( 5 ) ) {
											// if difference is bigger than 5 seconds something is off, report
											job.Status = "Large time difference between expected (" + expectedVideoLength.ToString() + ") and actual (" + actualVideoLength.ToString() + "), reenqueueing. (" + file + ")";
											job.JobStatus = VideoJobStatus.NotStarted;
										} else {
											job.Status = "Sane.";
										}
									} catch ( Exception ex ) {
										job.Status = "Exception while sanity checking: " + ex.ToString();
										job.JobStatus = VideoJobStatus.NotStarted;
									}
								}
							}
						}

					} catch ( Exception ) {
					}
				}
			}
		}

		private void buttonQueueSettings_Click( object sender, EventArgs e ) {
			ContextMenuStrip menu = new ContextMenuStrip();


			{
				ToolStripMenuItem item = new ToolStripMenuItem("Enqueue All");
				item.Click += (lsender, le) => {
					EnqueueAll();
				};
				menu.Items.Add(item);
			}

			{
				ToolStripMenuItem item = new ToolStripMenuItem("Enqueue Only...");
				foreach (StreamService service in Enum.GetValues(typeof(StreamService))) {
					item.DropDownItems.Add(service.ToString()).Click += (lsender, le) => {
						EnqueueAll(service);
					};
				}
				menu.Items.Add(item);
			}

			menu.Items.Add(new ToolStripSeparator());

			{
				ToolStripMenuItem item = new ToolStripMenuItem("Dequeue All");
				item.Click += (lsender, le) => {
					DequeueAll();
				};
				menu.Items.Add(item);
			}

			{
				ToolStripMenuItem item = new ToolStripMenuItem("Dequeue Only...");
				foreach (StreamService service in Enum.GetValues(typeof(StreamService))) {
					item.DropDownItems.Add(service.ToString()).Click += (lsender, le) => {
						DequeueAll(service);
					};
				}
				menu.Items.Add(item);
			}

			menu.Items.Add(new ToolStripSeparator());

			{
				ToolStripMenuItem item = new ToolStripMenuItem("Auto-enqueue...");
				foreach (StreamService service in Enum.GetValues(typeof(StreamService))) {
					ToolStripMenuItem tsi = new ToolStripMenuItem(service.ToString());
					bool autoenqueue = VideoTaskGroups[service].AutoEnqueue;
					tsi.Checked = autoenqueue;
					tsi.Click += (lsender, le) => {
						VideoTaskGroups[service].AutoEnqueue = !autoenqueue;
					};
					item.DropDownItems.Add(tsi);
				}
				menu.Items.Add(item);
			}

			menu.Show(Cursor.Position);
		}

		private void EnqueueAll(StreamService? service = null) {
			foreach (var item in objectListViewDownloads.Objects) {
				IVideoJob job = item as IVideoJob;
				if (job != null && job.JobStatus == VideoJobStatus.NotStarted) {
					if (service == null || service.Value == job.VideoInfo.Service) {
						VideoTaskGroups[job.VideoInfo.Service].Add(new WaitingVideoJob(job));
					}
				}
			}
		}

		private void DequeueAll(StreamService? service = null) {
			foreach (var kvp in VideoTaskGroups) {
				if (service == null || service.Value == kvp.Key) {
					kvp.Value.DequeueAll();
				}
			}
		}

		private void buttonOpenLog_Click(object sender, EventArgs e) {
			StringBuilder sb = new StringBuilder();
			lock (StatusMessageLock) {
				foreach (string msg in StatusMessages) {
					sb.AppendLine(msg);
				}
			}
			new LogView(sb.ToString()).Show();
		}
	}
}
