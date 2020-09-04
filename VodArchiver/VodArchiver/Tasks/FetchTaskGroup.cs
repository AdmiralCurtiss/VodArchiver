using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using VodArchiver.UserInfo;
using VodArchiver.VideoInfo;

namespace VodArchiver.Tasks {
	public class FetchTaskGroup {
		private Task Runner;
		private object ContainerLock;
		private List<IUserInfo> UserInfos;
		private Random RNG = new Random();

		private DownloadForm Form;
		private CancellationToken CancelToken;

		public FetchTaskGroup(DownloadForm form, CancellationToken cancellationToken) {
			Form = form;
			CancelToken = cancellationToken;
			ContainerLock = new object();
			UserInfos = new List<IUserInfo>();

			Runner = Run();
		}

		public void Add(IUserInfo userInfo) {
			lock (ContainerLock) {
				if (userInfo.AutoDownload) {
					UserInfos.Add(userInfo);
				}
			}
		}

		private async Task Run() {
			while (true) {
				try {
					await Task.Delay(3000, CancelToken);
				} catch (Exception) { }

				if (CancelToken.IsCancellationRequested) {
					return;
				}

				try {
					IUserInfo earliestUserInfo = null;
					lock (ContainerLock) {
						foreach (IUserInfo u in UserInfos) {
							if (u.AutoDownload && (earliestUserInfo == null || u.LastRefreshedOn < earliestUserInfo.LastRefreshedOn)) {
								earliestUserInfo = u;
							}
						}
					}

					if (earliestUserInfo != null) {
						DateTime earliestStartTime = earliestUserInfo.LastRefreshedOn.AddHours(7.0);
						DateTime now = DateTime.UtcNow;
						if (earliestStartTime <= now) {
							try {
								await DoFetch(RNG, earliestUserInfo, Form, CancelToken);
							} finally {
								earliestUserInfo.LastRefreshedOn = now;
								if (earliestUserInfo.Persistable) {
									UserInfoPersister.AddOrUpdate(earliestUserInfo);
									UserInfoPersister.Save();
								}
							}
						}
					}
				} catch (Exception ex) {
					Form.AddStatusMessage("Error during fetch: " + ex.ToString());
				}
			}
		}

		private static async Task DoFetch(Random rng, IUserInfo userInfo, DownloadForm downloadWindow, CancellationToken cancellationToken) {
			List<IVideoInfo> videos = new List<IVideoInfo>();

			while (true) {
				downloadWindow.AddStatusMessage("Fetching " + userInfo.ToString() + "...");

				try {
					FetchReturnValue fetchReturnValue;
					int Offset = 0;
					do {
						await Task.Delay(rng.Next(55000, 95000));
						fetchReturnValue = await userInfo.Fetch(Offset, true);
						Offset += fetchReturnValue.VideoCountThisFetch;
						if (fetchReturnValue.Success) {
							videos.AddRange(fetchReturnValue.Videos);
						}
					} while (fetchReturnValue.Success && fetchReturnValue.HasMore);
					break;
				} catch (Exception ex) {
					downloadWindow.AddStatusMessage("Error during " + userInfo.ToString() + ": " + ex.ToString());
					break;
				}
			}

			await Task.Delay(rng.Next(55000, 95000));
			DownloadFetched(videos, downloadWindow);
		}

		private static void DownloadFetched(IEnumerable<IVideoInfo> videos, DownloadForm downloadWindow) {
			Console.WriteLine("Enqueueing " + videos.Count() + " videos...");
			foreach (IVideoInfo videoInfo in videos) {
				Console.WriteLine("Enqueueing " + videoInfo.Username + "/" + videoInfo.VideoId);
				downloadWindow.CreateAndEnqueueJob(videoInfo);
			}
		}

		public async Task WaitForFetchThreadToEnd() {
			await Runner;
		}
	}
}
