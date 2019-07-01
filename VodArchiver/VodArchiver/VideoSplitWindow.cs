using System;
using System.Collections.Generic;
using System.IO;
using System.Windows.Forms;
using VodArchiver.VideoJobs;

namespace VodArchiver
{
	public partial class VideoSplitWindow : Form
	{
		DownloadForm downloadForm;

		public VideoSplitWindow( DownloadForm downloadForm ) {
			this.downloadForm = downloadForm;
			InitializeComponent();
		}

		private void buttonBrowse_Click( object sender, EventArgs e ) {
			OpenFileDialog d = new OpenFileDialog();
			if ( d.ShowDialog() == DialogResult.OK ) {
				this.textBoxFilename.Text = d.FileName;
			}
		}

		private async void buttonSplit_Click( object sender, EventArgs e ) {
			if ( !this.buttonSplit.Enabled || this.textBoxFilename.Text == "" || this.textBoxSplits.Text == "" ) {
				return;
			}

			try {
				this.buttonSplit.Enabled = false;

				downloadForm.EnqueueJob( new FFMpegSplitJob( this.textBoxFilename.Text, this.textBoxSplits.Text ) );

				this.textBoxFilename.Text = "";
				this.textBoxSplits.Text = "";
			} finally {
				this.buttonSplit.Enabled = true;
			}
		}
	}
}
