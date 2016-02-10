using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using TwixelAPI;
using VodArchiver.VideoJobs;

namespace VodArchiver {
	public partial class Form1 : Form {
		Twixel TwitchAPI;
		public Form1() {
			InitializeComponent();
			comboBoxService.SelectedIndex = 0;
			TwitchAPI = new Twixel( "", "", Twixel.APIVersion.v3 );
		}

		private async void buttonDownload_Click( object sender, EventArgs e ) {
			string id = textboxMediaId.Text.Trim();
			if ( id == "" ) { return; }

			IVideoJob job;
			switch ( comboBoxService.Text ) {
				case "Twitch":
					job = new TwitchVideoJob( TwitchAPI, id );
					break;
				case "Hitbox":
					job = new HitboxVideoJob( id );
					break;
				default:
					throw new Exception( comboBoxService.SelectedText + " is not a valid service." );
			}

			await job.Run();
		}
	}
}
