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

			comboBoxKnownUsers.Items.Add( " == No Preset == " );
			comboBoxKnownUsers.Items.AddRange( UserInfoPersister.KnownUsers.ToArray() );
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

		struct FetchReturnValue { public bool Success; public bool HasMore; public long TotalVideos; public int VideoCountThisFetch; public List<IVideoInfo> Videos; }
		private async Task<FetchReturnValue> Fetch() {
			UserInfo userInfo = new UserInfo();
			switch ( comboBoxService.Text ) {
				case "Twitch (Recordings)": userInfo.Service = ServiceVideoCategoryType.TwitchRecordings; break;
				case "Twitch (Highlights)": userInfo.Service = ServiceVideoCategoryType.TwitchHighlights; break;
				case "Hitbox": userInfo.Service = ServiceVideoCategoryType.HitboxRecordings; break;
				default: return new FetchReturnValue { Success = false, HasMore = false };
			}
			userInfo.Username = textboxUsername.Text.Trim();

			var rv = await Fetch( TwitchAPI, userInfo, Offset );

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

		private static async Task<FetchReturnValue> Fetch( Twixel twitchApi, UserInfo userInfo, int offset ) {
			List<IVideoInfo> videosToAdd = new List<IVideoInfo>();
			bool hasMore = true;
			long maxVideos = -1;
			int currentVideos = -1;

			switch ( userInfo.Service ) {
				case ServiceVideoCategoryType.TwitchRecordings:
				case ServiceVideoCategoryType.TwitchHighlights:
					Total<List<Video>> broadcasts = await twitchApi.RetrieveVideos( userInfo.Username, offset: offset, limit: 25, broadcasts: userInfo.Service == ServiceVideoCategoryType.TwitchRecordings, hls: false );
					if ( broadcasts.total.HasValue ) {
						hasMore = offset + broadcasts.wrapped.Count < broadcasts.total;
						maxVideos = (long)broadcasts.total;
					} else {
						hasMore = broadcasts.wrapped.Count == 25;
					}
					foreach ( var v in broadcasts.wrapped ) {
						videosToAdd.Add( new TwitchVideoInfo( v, StreamService.Twitch ) );
						videosToAdd.Add( new TwitchVideoInfo( v, StreamService.TwitchChatReplay ) );
					}
					currentVideos = broadcasts.wrapped.Count;
					break;
				case ServiceVideoCategoryType.HitboxRecordings:
					List<HitboxVideo> videos = await Hitbox.RetrieveVideos( userInfo.Username, offset: offset, limit: 100 );
					hasMore = videos.Count == 100;
					foreach ( var v in videos ) {
						videosToAdd.Add( new HitboxVideoInfo( v ) );
					}
					currentVideos = videos.Count;
					break;
				default:
					return new FetchReturnValue { Success = false, HasMore = false, TotalVideos = maxVideos, VideoCountThisFetch = 0, Videos = videosToAdd };
			}

			if ( videosToAdd.Count <= 0 ) {
				return new FetchReturnValue { Success = true, HasMore = false, TotalVideos = maxVideos, VideoCountThisFetch = 0, Videos = videosToAdd };
			}

			if ( UserInfoPersister.KnownUsers.Add( userInfo ) ) {
				UserInfoPersister.Save();
			}

			return new FetchReturnValue { Success = true, HasMore = hasMore, TotalVideos = maxVideos, VideoCountThisFetch = currentVideos, Videos = videosToAdd };
		}

		private void objectListViewVideos_ButtonClick( object sender, BrightIdeasSoftware.CellClickEventArgs e ) {
			IVideoInfo videoInfo = (IVideoInfo)e.Model;
			DownloadWindow.CreateAndEnqueueJob( videoInfo );
			Task.Run( () => DownloadWindow.RunJob() );
		}

		private void comboBoxKnownUsers_SelectedIndexChanged( object sender, EventArgs e ) {
			ProcessSelectedPreset();
		}

		private void ProcessSelectedPreset() {
			ProcessPreset( comboBoxKnownUsers.SelectedItem as UserInfo );
		}

		private void ProcessPreset( UserInfo u ) {
			if ( u != null ) {
				comboBoxService.SelectedIndex = (int)u.Service;
				textboxUsername.Text = u.Username;

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
				if ( videoInfo.VideoType == VideoFileType.M3U ) {
					downloadWindow.CreateAndEnqueueJob( videoInfo );
				}
				for ( int i = 0; i < DownloadForm.MaxRunningJobs; ++i ) {
					Task.Run( () => downloadWindow.RunJob() );
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
			List<UserInfo> users = new List<UserInfo>( comboBoxKnownUsers.Items.Count - 1 );
			for ( int i = 1; i < comboBoxKnownUsers.Items.Count; ++i ) {
				comboBoxKnownUsers.SelectedIndex = i;
				var userInfo = comboBoxKnownUsers.SelectedItem as UserInfo;
				if ( userInfo != null ) {
					users.Add( userInfo );
				}
			}
			await AutoDownload( users.ToArray(), TwitchAPI, DownloadWindow );
		}

		public static async Task AutoDownload( UserInfo[] users, Twixel twitchApi, DownloadForm downloadWindow ) {
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
								fetchReturnValue = await Fetch( twitchApi, userInfo, Offset );
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
			UserInfo u = comboBoxKnownUsers.SelectedItem as UserInfo;
			if ( u != null ) {
				u.AutoDownload = checkBoxAutoDownload.Checked;
				if ( UserInfoPersister.KnownUsers.AddOrUpdate( u ) ) {
					UserInfoPersister.Save();
				}
			}
		}
	}
}
