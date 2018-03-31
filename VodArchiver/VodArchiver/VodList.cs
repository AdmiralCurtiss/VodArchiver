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
using VodArchiver.UserInfo;
using VodArchiver.VideoInfo;

namespace VodArchiver {
	public partial class VodList : Form {
		DownloadForm DownloadWindow;
		Twixel TwitchAPI;
		int Offset = 0;
		IUserInfo selectedUserInfo = null;

		public VodList( DownloadForm parent, Twixel twixel ) {
			InitializeComponent();
			DownloadWindow = parent;
			TwitchAPI = twixel;
			comboBoxService.Text = "Twitch (Recordings)";
			objectListViewVideos.SecondarySortColumn = objectListViewVideos.GetColumn( "Video ID" );
			objectListViewVideos.SecondarySortOrder = SortOrder.Ascending;

			comboBoxKnownUsers.Items.Add( " == No Preset == " );
			comboBoxKnownUsers.Items.AddRange( UserInfoPersister.GetKnownUsers().ToArray() );
			comboBoxKnownUsers.SelectedIndex = 0;

			buttonClear.Enabled = false;
			checkBoxAutoDownload.Enabled = false;

			if ( !Util.ShowDownloadFetched ) {
				buttonDownloadFetched.Enabled = false;
				buttonDownloadFetched.Hide();
				buttonDownloadAllKnown.Enabled = false;
				buttonDownloadAllKnown.Hide();
				checkBoxAutoDownload.Hide();
			}
			if ( !Util.ShowAnySpecialButton ) {
				objectListViewVideos.Size = new Size( objectListViewVideos.Size.Width, objectListViewVideos.Size.Height + 29 );
			}
		}

		private async void buttonFetch_Click( object sender, EventArgs e ) {
			try {
				var retVal = await Fetch();
				if ( !retVal.Success ) {
					MessageBox.Show( "Fetch failed!" );
				}
			} catch ( Exception ex ) {
				MessageBox.Show( ex.ToString() );
			}
		}

		private async Task<FetchReturnValue> Fetch() {
			IUserInfo userInfo;
			if ( selectedUserInfo == null ) {
				GenericUserInfo ui = new GenericUserInfo();
				switch ( comboBoxService.Text ) {
					case "Twitch (Recordings)": ui.Service = ServiceVideoCategoryType.TwitchRecordings; break;
					case "Twitch (Highlights)": ui.Service = ServiceVideoCategoryType.TwitchHighlights; break;
					case "Hitbox": ui.Service = ServiceVideoCategoryType.HitboxRecordings; break;
					case "Youtube (Playlist)": ui.Service = ServiceVideoCategoryType.YoutubePlaylist; break;
					case "Youtube (User)": ui.Service = ServiceVideoCategoryType.YoutubeUser; break;
					case "Youtube (Channel)": ui.Service = ServiceVideoCategoryType.YoutubeChannel; break;
					case "RSS Feed": ui.Service = ServiceVideoCategoryType.RssFeed; break;
					case "FFMpeg Reencode": ui.Service = ServiceVideoCategoryType.FFMpegJob; break;
					default: return new FetchReturnValue { Success = false, HasMore = false };
				}
				ui.Persistable = checkBoxSaveForLater.Checked;
				ui.Username = textboxUsername.Text.Trim();
			
				if ( ui.Service == ServiceVideoCategoryType.YoutubePlaylist && ( ui.Username.StartsWith( "http://" ) || ui.Username.StartsWith( "https://" ) ) ) {
					ui.Username = Util.GetParameterFromUri( new Uri( ui.Username ), "list" );
				}
				userInfo = ui;
			} else {
				userInfo = selectedUserInfo;
			}

			var rv = await Fetch( TwitchAPI, userInfo, Offset, checkBoxFlat.Checked );

			if ( rv.Success && rv.Videos.Count > 0 ) {
				Offset += rv.VideoCountThisFetch;
				textboxUsername.Enabled = false;
				comboBoxService.Enabled = false;
				comboBoxKnownUsers.Enabled = false;
				if ( rv.HasMore && rv.TotalVideos != -1 ) {
					buttonFetch.Text = "Fetch More (" + ( rv.TotalVideos - Offset ) + " left)";
				} else {
					buttonFetch.Text = "Fetch More";
				}
				buttonFetch.Enabled = rv.HasMore;
				buttonClear.Enabled = true;

				objectListViewVideos.BeginUpdate();
				foreach ( var v in rv.Videos ) {
					objectListViewVideos.AddObject( v );
				}
				objectListViewVideos.EndUpdate();
			}

			return rv;
		}

		private static async Task<FetchReturnValue> Fetch( Twixel twitchApi, IUserInfo userInfo, int offset, bool flat ) {
			FetchReturnValue frv = await userInfo.Fetch( twitchApi, offset, flat );

			if ( !frv.Success ) {
				return frv;
			}

			userInfo.LastRefreshedOn = DateTime.UtcNow;

			if ( userInfo.Persistable ) {
				UserInfoPersister.AddOrUpdate( userInfo );
				UserInfoPersister.Save();
			}

			return frv;
		}

		private void objectListViewVideos_ButtonClick( object sender, BrightIdeasSoftware.CellClickEventArgs e ) {
			IVideoInfo videoInfo = (IVideoInfo)e.Model;
			DownloadWindow.CreateAndEnqueueJob( videoInfo );
		}

		private void comboBoxKnownUsers_SelectedIndexChanged( object sender, EventArgs e ) {
			ProcessSelectedPreset();
		}

		private void ProcessSelectedPreset() {
			ProcessPreset( comboBoxKnownUsers.SelectedItem as IUserInfo );
		}

		private void ProcessPreset( IUserInfo u ) {
			selectedUserInfo = u;
			if ( u != null ) {
				comboBoxService.SelectedIndex = (int)u.Type;
				textboxUsername.Text = u.UserIdentifier;

				comboBoxService.Enabled = false;
				textboxUsername.Enabled = false;
				if ( Util.ShowDownloadFetched ) {
					checkBoxAutoDownload.Enabled = true;
					checkBoxAutoDownload.Checked = u.AutoDownload;
				}
			} else {
				comboBoxService.Enabled = true;
				textboxUsername.Enabled = true;
				if ( Util.ShowDownloadFetched ) {
					checkBoxAutoDownload.Enabled = false;
					checkBoxAutoDownload.Checked = false;
				}
			}
		}

		private void buttonDownloadFetched_Click( object sender, EventArgs e ) {
			DownloadFetched();
		}

		private void DownloadFetched() {
			if ( objectListViewVideos.GetItemCount() <= 0 ) {
				return;
			}

			List<IVideoInfo> videos = new List<IVideoInfo>();
			foreach ( var o in objectListViewVideos.Objects ) {
				videos.Add( o as IVideoInfo );
			}
			DownloadFetched( videos, DownloadWindow );
		}

		public static void DownloadFetched( IEnumerable<IVideoInfo> videos, DownloadForm downloadWindow ) {
			Console.WriteLine( "Enqueueing " + videos.Count() + " videos..." );
			foreach ( IVideoInfo videoInfo in videos ) {
				Console.WriteLine( "Enqueueing " + videoInfo.Username + "/" + videoInfo.VideoId );
				downloadWindow.CreateAndEnqueueJob( videoInfo );
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
			List<IUserInfo> users = new List<IUserInfo>( comboBoxKnownUsers.Items.Count - 1 );
			for ( int i = 1; i < comboBoxKnownUsers.Items.Count; ++i ) {
				comboBoxKnownUsers.SelectedIndex = i;
				var userInfo = comboBoxKnownUsers.SelectedItem as IUserInfo;
				if ( userInfo != null ) {
					users.Add( userInfo );
				}
			}
			await AutoDownload( users.ToArray(), TwitchAPI, DownloadWindow );
		}

		public static async Task AutoDownload( IUserInfo[] users, Twixel twitchApi, DownloadForm downloadWindow ) {
			Random rng = new Random();
			for ( int i = 0; i < users.Length; ++i ) {
				var userInfo = users[i];
				if ( userInfo != null ) {
					if ( !userInfo.AutoDownload ) {
						continue;
					}

					List<IVideoInfo> videos = new List<IVideoInfo>();

					while ( true ) {
						downloadWindow.SetAutoDownloadStatus( "[" + ( i + 1 ).ToString() + "/" + users.Length.ToString() + "] Fetching " + userInfo.ToString() + "..." );

						try {
							FetchReturnValue fetchReturnValue;
							int Offset = 0;
							do {
								await Task.Delay( rng.Next( 55000, 95000 ) );
								fetchReturnValue = await Fetch( twitchApi, userInfo, Offset, true );
								Offset += fetchReturnValue.VideoCountThisFetch;
								if ( fetchReturnValue.Success ) {
									videos.AddRange( fetchReturnValue.Videos );
								}
							} while ( fetchReturnValue.Success && fetchReturnValue.HasMore );
							break;
						} catch ( Exception ex ) {
							// TODO: Better errorhandling?
							break;
						}
					}

					await Task.Delay( rng.Next( 55000, 95000 ) );
					DownloadFetched( videos, downloadWindow );
				}
			}
		}

		private void checkBoxAutoDownload_CheckedChanged( object sender, EventArgs e ) {
			IUserInfo u = comboBoxKnownUsers.SelectedItem as IUserInfo;
			if ( u != null ) {
				if ( u.AutoDownload != checkBoxAutoDownload.Checked ) {
					u.AutoDownload = checkBoxAutoDownload.Checked;
					if ( u.Persistable ) {
						UserInfoPersister.AddOrUpdate( u );
						UserInfoPersister.Save();
					}
				}
			}
		}
	}
}
