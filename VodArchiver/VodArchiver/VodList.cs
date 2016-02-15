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
		}

		private async void buttonDownload_Click( object sender, EventArgs e ) {
			Total<List<Video>> broadcasts = await TwitchAPI.RetrieveVideos( textboxUsername.Text.Trim(), 0, 25, true, false );
			//Total<List<Video>> highlights = await TwitchAPI.RetrieveVideos( textboxUsername.Text.Trim(), 0, 25, false, false );

			foreach ( var v in broadcasts.wrapped ) {
				objectListViewVideos.AddObject( new TwitchVideoInfo( v ) );
			}
		}

		private async void objectListViewVideos_ButtonClick( object sender, BrightIdeasSoftware.CellClickEventArgs e ) {
			IVideoInfo videoInfo = (IVideoInfo)e.Model;
			DownloadWindow.CreateAndEnqueueJob( videoInfo.Service, videoInfo.VideoId );
			await DownloadWindow.RunJob();
		}
	}
}
