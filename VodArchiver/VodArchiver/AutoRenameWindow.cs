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

		private async void buttonGenDurationDiff_Click(object sender, EventArgs e) {
			string path = textBoxPath.Text.Trim();
			if (path != "") {
				StringBuilder sb = new StringBuilder();
				await GenDurationTwitch(sb, path);
				textBoxDurationDiff.Text = sb.ToString();
			}
		}

		private async Task GenDurationTwitch(StringBuilder sb, string path) {
			sb.AppendFormat("Scanning {0}...", path).AppendLine();
			List<string> filesInDir = Directory.EnumerateFiles(path).ToList();

			foreach (string filepath in filesInDir) {
				string filename = Path.GetFileName(filepath);

				foreach (IVideoJob job in Jobs) {
					if (job != null && job.VideoInfo.Service == StreamService.Twitch) {
						string filestart = "twitch_" + job.VideoInfo.Username + "_";
						string filemid = "_v" + job.VideoInfo.VideoId + "_";
						string fileend = ".mp4";
						if (filename.StartsWith(filestart) && filename.Contains(filemid) && filename.EndsWith(fileend)) {
							Console.WriteLine("Probing {0}", filepath);
							TimeSpan actualVideoLength = (await FFMpegUtil.Probe(filepath)).Duration;
							TimeSpan expectedVideoLength = job.VideoInfo.VideoLength;
							if (actualVideoLength.Subtract(expectedVideoLength).Duration() > TimeSpan.FromSeconds(5)) {
								sb.AppendFormat("File {0} seems to have a large difference. ({1} seconds)", filepath, actualVideoLength.Subtract(expectedVideoLength).Duration().TotalSeconds);
								sb.AppendLine();
							}
						}
					}
				}
			}

			try {
				foreach (string subdir in Directory.EnumerateDirectories(path)) {
					try {
						await GenDurationTwitch(sb, subdir);
					} catch (Exception) { }
				}
			} catch (Exception) { }
		}

		private async void buttonGenMeta_Click(object sender, EventArgs e) {
			string path = textBoxPath.Text.Trim();
			if (path != "") {
				StringBuilder sb = new StringBuilder();
				await GenMetadataOutputForFiles(sb, path);
				textBoxDurationDiff.Text = sb.ToString();
			}
		}

		private static void AppendCsvRow(StringBuilder sb, List<string> stuff) {
			bool first = true;
			foreach (string s in stuff) {
				if (first) {
					first = false;
				} else {
					sb.Append(',');
				}
				if (s == null) {
					continue;
				}
				if (s.Any(x => x == '"' || x == '\0' || x == ',' || x == '\n' || x == '\\' || x == '\'')) {
					sb.Append('"').Append(s.Replace("\"", "\"\"")).Append('"');
				} else {
					sb.Append(s);
				}
			}
			sb.AppendLine();
		}

		private async Task GenMetadataOutputForFiles(StringBuilder sb, string path) {
			sb.AppendFormat("Scanning {0}...", path).AppendLine();
			List<string> filesInDir = Directory.EnumerateFiles(path).ToList();

			foreach (string filepath in filesInDir) {
				string filename = Path.GetFileName(filepath);

				foreach (IVideoJob job in Jobs) {
					if (job != null && (job.VideoInfo.Service == StreamService.Twitch || job.VideoInfo.Service == StreamService.TwitchChatReplay)) {
						string filestart = "twitch_" + job.VideoInfo.Username + "_";
						string filemid = "_v" + job.VideoInfo.VideoId + "_";
						string fileend = ".mp4";
						bool typeMatches = filename.EndsWith(fileend) == (job.VideoInfo.Service == StreamService.Twitch);
						if (typeMatches && filename.StartsWith(filestart) && filename.Contains(filemid)) {
							TimeSpan? actualVideoLength = null;
							if (filename.EndsWith(fileend)) {
								Console.WriteLine("Probing {0}", filepath);
								actualVideoLength = (await FFMpegUtil.Probe(filepath)).Duration;
							}

							List<string> stuff = new List<string>();
							stuff.Add(filename);
							stuff.Add(actualVideoLength.HasValue ? actualVideoLength.Value.TotalSeconds.ToString(System.Globalization.CultureInfo.InvariantCulture) : "");
							stuff.Add(job.VideoInfo.VideoId);
							stuff.Add(job.VideoInfo.VideoTitle);
							stuff.Add(job.VideoInfo.VideoGame);
							stuff.Add((job.VideoInfo as TwitchVideoInfo)?._Video?.Description ?? "");
							stuff.Add(job.VideoInfo.VideoTimestamp.ToString("s", System.Globalization.CultureInfo.InvariantCulture));
							stuff.Add(new FileInfo(filepath).Length.ToString());
							AppendCsvRow(sb, stuff);
						}
					}
				}
			}

			try {
				foreach (string subdir in Directory.EnumerateDirectories(path)) {
					try {
						await GenDurationTwitch(sb, subdir);
					} catch (Exception) { }
				}
			} catch (Exception) { }
		}
	}
}
