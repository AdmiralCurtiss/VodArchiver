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

		public VodList( DownloadForm parent, Twixel twixel ) {
			InitializeComponent();
			DownloadWindow = parent;
			TwitchAPI = twixel;
			comboBoxService.Text = "Twitch (Recordings)";
		}

		private async void buttonFetch_Click( object sender, EventArgs e ) {
			List<IVideoInfo> videosToAdd = new List<IVideoInfo>();
			switch ( comboBoxService.Text ) {
				case "Twitch (Recordings)":
				case "Twitch (Highlights)":
					Total<List<Video>> broadcasts = await TwitchAPI.RetrieveVideos( textboxUsername.Text.Trim(), 0, 25, comboBoxService.Text == "Twitch (Recordings)", false );
					foreach ( var v in broadcasts.wrapped ) {
						videosToAdd.Add( new TwitchVideoInfo( v ) );
					}
					break;
				case "Hitbox":
					List<HitboxVideo> videos = await Hitbox.RetrieveVideos( textboxUsername.Text.Trim() );
					foreach ( var v in videos ) {
						videosToAdd.Add( new HitboxVideoInfo( v ) );
					}
					break;
			}

			objectListViewVideos.SuspendLayout();
			foreach ( var v in videosToAdd ) {
				objectListViewVideos.AddObject( v );
			}
			objectListViewVideos.ResumeLayout();
		}

		private async void objectListViewVideos_ButtonClick( object sender, BrightIdeasSoftware.CellClickEventArgs e ) {
			IVideoInfo videoInfo = (IVideoInfo)e.Model;
			DownloadWindow.CreateAndEnqueueJob( videoInfo.Service, videoInfo.VideoId );
			await DownloadWindow.RunJob();
		}
	}
}
