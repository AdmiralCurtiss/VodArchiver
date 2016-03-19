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

			if ( !Util.ShowDownloadFetched ) {
				buttonDownloadFetched.Enabled = false;
				buttonDownloadFetched.Hide();
			}
			if ( !Util.ShowAnySpecialButton ) {
				objectListViewVideos.Size = new Size( objectListViewVideos.Size.Width, objectListViewVideos.Size.Height + 29 );
			}
		}

		private async void buttonFetch_Click( object sender, EventArgs e ) {
			List<IVideoInfo> videosToAdd = new List<IVideoInfo>();
			bool hasMore = true;
			long maxVideos = -1;

			UserInfo userInfo = new UserInfo();
			switch ( comboBoxService.Text ) {
				case "Twitch (Recordings)": userInfo.Service = ServiceVideoCategoryType.TwitchRecordings; break;
				case "Twitch (Highlights)": userInfo.Service = ServiceVideoCategoryType.TwitchHighlights; break;
				case "Hitbox": userInfo.Service = ServiceVideoCategoryType.HitboxRecordings; break;
				default: return;
			}
			userInfo.Username = textboxUsername.Text.Trim();

			try {
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
			} catch ( Exception ex ) {
				MessageBox.Show( ex.ToString() );
			}

			if ( videosToAdd.Count <= 0 ) {
				return;
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

			objectListViewVideos.BeginUpdate();
			foreach ( var v in videosToAdd ) {
				objectListViewVideos.AddObject( v );
			}
			objectListViewVideos.EndUpdate();
		}

		private void objectListViewVideos_ButtonClick( object sender, BrightIdeasSoftware.CellClickEventArgs e ) {
			IVideoInfo videoInfo = (IVideoInfo)e.Model;
			DownloadWindow.CreateAndEnqueueJob( videoInfo.Service, videoInfo.VideoId );
			Task.Run( () => DownloadWindow.RunJob() );
		}

		private void comboBoxKnownUsers_SelectedIndexChanged( object sender, EventArgs e ) {
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
	}
}
