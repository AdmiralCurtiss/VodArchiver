﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using TwixelAPI;
using VodArchiver.VideoInfo;

namespace VodArchiver.Tasks {
	public class FetchTaskGroup {
		private Task Runner;
		private object ContainerLock;
		private List<UserInfo> UserInfos;

		private Twixel TwitchAPI;
		private DownloadForm Form;

		public FetchTaskGroup( Twixel twitchApi, DownloadForm form ) {
			TwitchAPI = twitchApi;
			Form = form;
			ContainerLock = new Object();
			UserInfos = new List<UserInfo>();

			Runner = Run();
		}

		public void Add( UserInfo userInfo ) {
			lock ( ContainerLock ) {
				if ( userInfo.AutoDownload ) {
					UserInfos.Add( userInfo );
				}
			}
		}

		private async Task Run() {
			while ( true ) {
				try {
					await Task.Delay( 3000 );
				} catch ( Exception ) { }

				try {
					UserInfo earliestUserInfo = null;
					lock ( ContainerLock ) {
						foreach ( UserInfo u in UserInfos ) {
							if ( u.AutoDownload && ( earliestUserInfo == null || u.LastRefreshedOn < earliestUserInfo.LastRefreshedOn ) ) {
								earliestUserInfo = u;
							}
						}
					}

					if ( earliestUserInfo != null ) {
						DateTime earliestStartTime = earliestUserInfo.LastRefreshedOn.AddHours( 7.0 );
						if ( earliestStartTime <= DateTime.UtcNow ) {
							// hacky, TODO: refactor
							await VodList.AutoDownload( new UserInfo[] { earliestUserInfo }, TwitchAPI, Form );
						}
					}
				} catch ( Exception ) { }
			}
		}
	}
}