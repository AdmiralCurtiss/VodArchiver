using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using TwixelAPI;
using VodArchiver.VideoInfo;

namespace VodArchiver {
	public partial class VodList : Form {
		DownloadForm DownloadWindow;
		Twixel TwitchAPI;
		int Offset = 0;

		public VodList( DownloadForm parent, Twixel twixel ) {
			InitializeComponent();
			DownloadWindow = parent;
			TwitchAPI = twixel;
			comboBoxService.Text = "Twitch (Recordings)";
			objectListViewVideos.SecondarySortColumn = objectListViewVideos.GetColumn( "Video ID" );
			objectListViewVideos.SecondarySortOrder = SortOrder.Ascending;

			if ( UserInfoPersister.KnownUsers.Count == 0 ) {
				try {
					UserInfoPersister.Load();
				} catch ( Exception ex ) {
					MessageBox.Show( "Couldn't load users." + Environment.NewLine + ex.ToString() );
				}
			}

			comboBoxKnownUsers.Items.Add( " == No Preset == " );
			comboBoxKnownUsers.Items.AddRange( UserInfoPersister.KnownUsers.ToArray() );
			comboBoxKnownUsers.SelectedIndex = 0;

			buttonClear.Enabled = false;

			if ( !Util.ShowDownloadFetched ) {
				buttonDownloadFetched.Enabled = false;
				buttonDownloadFetched.Hide();
				buttonDownloadAllKnown.Enabled = false;
				buttonDownloadAllKnown.Hide();
			}
			if ( !Util.ShowAnySpecialButton ) {
				objectListViewVideos.Size = new Size( objectListViewVideos.Size.Width, objectListViewVideos.Size.Height + 29 );
			}
		}

		private async void buttonFetch_Click( object sender, EventArgs e ) {
			try {
				await Fetch();
			} catch ( Exception ex ) {
				MessageBox.Show( ex.ToString() );
			}
		}

		private async Task<bool> Fetch() {
			List<IVideoInfo> videosToAdd = new List<IVideoInfo>();
			bool hasMore = true;
			long maxVideos = -1;

			UserInfo userInfo = new UserInfo();
			switch ( comboBoxService.Text ) {
				case "Twitch (Recordings)": userInfo.Service = ServiceVideoCategoryType.TwitchRecordings; break;
				case "Twitch (Highlights)": userInfo.Service = ServiceVideoCategoryType.TwitchHighlights; break;
				case "Hitbox": userInfo.Service = ServiceVideoCategoryType.HitboxRecordings; break;
				default: return false;
			}
			userInfo.Username = textboxUsername.Text.Trim();

			switch ( userInfo.Service ) {
				case ServiceVideoCategoryType.TwitchRecordings:
				case ServiceVideoCategoryType.TwitchHighlights:
					Total<List<Video>> broadcasts = await TwitchAPI.RetrieveVideos( userInfo.Username, offset: Offset, limit: 25, broadcasts: userInfo.Service == ServiceVideoCategoryType.TwitchRecordings, hls: false );
					if ( broadcasts.total.HasValue ) {
						hasMore = Offset + broadcasts.wrapped.Count < broadcasts.total;
						maxVideos = (long)broadcasts.total;
					} else {
						hasMore = broadcasts.wrapped.Count == 25;
					}
					Offset += broadcasts.wrapped.Count;
					foreach ( var v in broadcasts.wrapped ) {
						videosToAdd.Add( new TwitchVideoInfo( v ) );
					}
					break;
				case ServiceVideoCategoryType.HitboxRecordings:
					List<HitboxVideo> videos = await Hitbox.RetrieveVideos( userInfo.Username, offset: Offset, limit: 100 );
					hasMore = videos.Count == 100;
					Offset += videos.Count;
					foreach ( var v in videos ) {
						videosToAdd.Add( new HitboxVideoInfo( v ) );
					}
					break;
			}

			if ( videosToAdd.Count <= 0 ) {
				return true;
			}

			if ( UserInfoPersister.KnownUsers.Add( userInfo ) ) {
				UserInfoPersister.Save();
			}

			textboxUsername.Enabled = false;
			comboBoxService.Enabled = false;
			comboBoxKnownUsers.Enabled = false;
			if ( hasMore && maxVideos != -1 ) {
				buttonFetch.Text = "Fetch More (" + ( maxVideos - Offset ) + " left)";
			} else {
				buttonFetch.Text = "Fetch More";
			}
			buttonFetch.Enabled = hasMore;
			buttonClear.Enabled = true;

			objectListViewVideos.BeginUpdate();
			foreach ( var v in videosToAdd ) {
				objectListViewVideos.AddObject( v );
			}
			objectListViewVideos.EndUpdate();

			return true;
		}

		private void objectListViewVideos_ButtonClick( object sender, BrightIdeasSoftware.CellClickEventArgs e ) {
			IVideoInfo videoInfo = (IVideoInfo)e.Model;
			DownloadWindow.CreateAndEnqueueJob( videoInfo.Service, videoInfo.VideoId );
			Task.Run( () => DownloadWindow.RunJob() );
		}

		private void comboBoxKnownUsers_SelectedIndexChanged( object sender, EventArgs e ) {
			ProcessSelectedPreset();
		}

		private void ProcessSelectedPreset() {
			UserInfo u = comboBoxKnownUsers.SelectedItem as UserInfo;
			if ( u != null ) {
				comboBoxService.SelectedIndex = (int)u.Service;
				textboxUsername.Text = u.Username;

				comboBoxService.Enabled = false;
				textboxUsername.Enabled = false;
			} else {
				comboBoxService.Enabled = true;
				textboxUsername.Enabled = true;
			}
		}

		private void buttonDownloadFetched_Click( object sender, EventArgs e ) {
			DownloadFetched();
		}

		private void DownloadFetched() {
			if ( objectListViewVideos.GetItemCount() <= 0 ) {
				return;
			}

			foreach ( var o in objectListViewVideos.Objects ) {
				IVideoInfo videoInfo = o as IVideoInfo;
				if ( videoInfo.VideoRecordingState == RecordingState.Recorded && videoInfo.VideoType == VideoFileType.M3U ) {
					DownloadWindow.CreateAndEnqueueJob( videoInfo.Service, videoInfo.VideoId );
				}
				for ( int i = 0; i < DownloadForm.MaxRunningJobs; ++i ) {
					Task.Run( () => DownloadWindow.RunJob() );
				}
			}
		}

		private void buttonClear_Click( object sender, EventArgs e ) {
			Clear();
		}

		private void Clear() {
			objectListViewVideos.ClearObjects();
			textboxUsername.Enabled = true;
			comboBoxService.Enabled = true;
			comboBoxKnownUsers.Enabled = true;
			buttonFetch.Enabled = true;
			buttonFetch.Text = "Fetch";
			buttonClear.Enabled = false;
			Offset = 0;
			ProcessSelectedPreset();
		}

		private async void buttonDownloadAllKnown_Click( object sender, EventArgs e ) {
			for ( int i = 1; i < comboBoxKnownUsers.Items.Count; ++i ) {
				comboBoxKnownUsers.SelectedIndex = i;
				if ( comboBoxKnownUsers.SelectedItem as UserInfo != null ) {
					ProcessSelectedPreset();
					// TODO: Fetch multipage
					while ( true ) {
						try {
							await Task.Delay( 15000 );
							await Fetch();
							break;
						} catch ( Exception ex ) {
							// TODO: Better errorhandling?
							var result = MessageBox.Show( ex.ToString(), "Exception", MessageBoxButtons.RetryCancel );
							if ( result == DialogResult.Cancel ) {
								break;
							}
						}
					}

					DownloadFetched();
					Clear();
				}
			}
		}
	}
}
