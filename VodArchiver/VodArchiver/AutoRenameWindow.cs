using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using VodArchiver.VideoInfo;
using VodArchiver.VideoJobs;

namespace VodArchiver {
	public partial class AutoRenameWindow : Form {
		private List<IVideoJob> Jobs;

		public AutoRenameWindow() {
			this.Jobs = new List<IVideoJob>();
			InitializeComponent();
		}

		public AutoRenameWindow(List<IVideoJob> jobs) {
			this.Jobs = jobs;
			InitializeComponent();
		}

		private void buttonRename_Click(object sender, EventArgs e) {
			string path = textBoxPath.Text.Trim();
			if (path != "") {
				RenameTwitch(path);
				RenameYoutube(path);
			}
		}

		private void RenameYoutube(string path) {
			foreach (IVideoJob job in Jobs) {
				if (job != null && job.VideoInfo.Service == StreamService.Youtube) {
					string origFilename = "youtube_" + job.VideoInfo.Username + "_" + job.VideoInfo.VideoTimestamp.ToString("yyyy-MM-dd") + "_" + job.VideoInfo.VideoId + ".mkv";
					string p = Path.Combine(path, origFilename);
					if (File.Exists(p)) {
						string finalFilename = "youtube_" + job.VideoInfo.Username + "_" + job.VideoInfo.VideoTimestamp.ToString("yyyy-MM-dd") + "_" + job.VideoInfo.VideoId + "_" + Util.MakeIntercapsFilename(job.VideoInfo.VideoTitle).Crop(80) + ".mkv";
						string q = Path.Combine(path, finalFilename);
						if (!File.Exists(q)) {
							Console.WriteLine("rename " + p + " -> " + q);
							File.Move(p, q);
						}
					}
				}
			}

			try {
				foreach (string subdir in Directory.EnumerateDirectories(path)) {
					try {
						RenameYoutube(subdir);
					} catch (Exception) { }
				}
			} catch (Exception) { }
		}

		private void RenameTwitch(string path) {
			string[] qualities = new string[] { "chunked", "x264crf23", "x264crf23scaled480p", "x264crf23scaled720p" };
			foreach (IVideoJob job in Jobs) {
				if (job != null && job.VideoInfo.Service == StreamService.Twitch) {
					foreach (string quality in qualities) {
						string origFilename0 = "twitch_" + job.VideoInfo.Username + "_v" + job.VideoInfo.VideoId + "_" + quality + ".mp4";
						string origFilename1 = "twitch_" + job.VideoInfo.Username + "_v" + job.VideoInfo.VideoId + "_" + Util.MakeIntercapsFilename(job.VideoInfo.VideoGame ?? "unknown") + "_" + quality + ".mp4";
						string origFilename2 = "twitch_" + job.VideoInfo.Username + "_v" + job.VideoInfo.VideoId + "_" + Util.MakeIntercapsFilename(job.VideoInfo.VideoGame ?? "unknown") + "_" + Util.MakeIntercapsFilename(job.VideoInfo.VideoTitle).Crop(80) + "_" + quality + ".mp4";
						string p0 = Path.Combine(path, origFilename0);
						string p1 = Path.Combine(path, origFilename1);
						string p2 = Path.Combine(path, origFilename2);
						bool p0e = File.Exists(p0);
						bool p1e = File.Exists(p1);
						bool p2e = File.Exists(p2);
						if (p0e || p1e || p2e) {
							string finalFilename = "twitch_" + job.VideoInfo.Username + "_" + job.VideoInfo.VideoTimestamp.ToString("yyyy-MM-dd_HH-mm-ss") + "_v" + job.VideoInfo.VideoId + "_" + Util.MakeIntercapsFilename(job.VideoInfo.VideoGame ?? "unknown") + "_" + Util.MakeIntercapsFilename(job.VideoInfo.VideoTitle).Crop(80) + "_" + quality + ".mp4";
							string q = Path.Combine(path, finalFilename);
							if (!File.Exists(q)) {
								string p = p0e ? p0 : p1e ? p1 : p2;
								Console.WriteLine("rename " + p + " -> " + q);
								File.Move(p, q);
							}
						}
					}
				}
			}
			try {
				foreach (string subdir in Directory.EnumerateDirectories(path)) {
					try {
						RenameTwitch(subdir);
					} catch (Exception) { }
				}
			} catch (Exception) { }
		}
	}
}
