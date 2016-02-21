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
		}

		private async void buttonFetch_Click( object sender, EventArgs e ) {
			List<IVideoInfo> videosToAdd = new List<IVideoInfo>();
			bool hasMore = true;
			long maxVideos = -1;

			try {
				switch ( comboBoxService.Text ) {
					case "Twitch (Recordings)":
					case "Twitch (Highlights)":
						Total<List<Video>> broadcasts = await TwitchAPI.RetrieveVideos( textboxUsername.Text.Trim(), offset: Offset, limit: 25, broadcasts: comboBoxService.Text == "Twitch (Recordings)", hls: false );
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
					case "Hitbox":
						List<HitboxVideo> videos = await Hitbox.RetrieveVideos( textboxUsername.Text.Trim(), offset: Offset, limit: 100 );
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

			textboxUsername.Enabled = false;
			comboBoxService.Enabled = false;
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
			Task.Run( DownloadWindow.RunJob );
		}
	}
}
