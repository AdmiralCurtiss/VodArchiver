﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using VodArchiver.VideoInfo;
using TwixelAPI;

namespace VodArchiver.VideoJobs {
	public class TwitchVideoJob : TsVideoJob {
		public override StreamService Service { get; set; }
		public override string Username { get; set; }
		public override string VideoId { get; set; }
		public override string VideoTitle { get; set; }
		public override string VideoGame { get; set; }
		public override DateTime VideoTimestamp { get; set; }
		public override TimeSpan VideoLength { get; set; }
		private string _Status;
		public override string Status {
			get {
				return _Status;
			}
			set {
				_Status = value;
				StatusUpdater.Update();
			}
		}

		public override StatusUpdate.IStatusUpdate StatusUpdater { get; set; }

		Twixel TwitchAPI;
		string VideoQuality = "chunked";
		TwixelAPI.Video VideoInfo = null;

		public TwitchVideoJob( Twixel api, string id, StatusUpdate.IStatusUpdate statusUpdater = null ) {
			StatusUpdater = statusUpdater == null ? new StatusUpdate.NullStatusUpdate() : statusUpdater;
			Service = StreamService.Twitch;
			Username = "...";
			VideoId = id;
			Status = "...";
			TwitchAPI = api;
		}

		public override async Task<string[]> GetFileUrlsOfVod() {
			VideoInfo = await TwitchAPI.RetrieveVideo( VideoId );
			Username = VideoInfo.channel["name"];
			VideoTitle = VideoInfo.title;
			VideoGame = VideoInfo.game;
			VideoTimestamp = VideoInfo.recordedAt;
			VideoLength = TimeSpan.FromSeconds( VideoInfo.length );

			string m3u = await TwitchAPI.RetrieveVodM3U( VideoId );
			string m3u8path = GetM3U8PathFromM3U( m3u, VideoQuality );
			string folderpath = TsVideoJob.GetFolder( m3u8path );
			string m3u8 = await Twixel.GetWebData( new Uri( m3u8path ) );
			string[] filenames = TsVideoJob.GetFilenamesFromM3U8( m3u8 );

			List<string> urls = new List<string>( filenames.Length );
			foreach ( var filename in filenames ) {
				urls.Add( folderpath + filename );
			}
			return urls.ToArray();
		}

		public static string GetM3U8PathFromM3U( string m3u, string videoType ) {
			var lines = m3u.Split( new char[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries );
			foreach ( var line in lines ) {
				if ( line.Trim() == "" || line.Trim().StartsWith( "#" ) ) {
					continue;
				}

				var urlParts = line.Trim().Split( '/' );
				urlParts[urlParts.Length - 2] = videoType;
				return String.Join( "/", urlParts );
			}

			throw new Exception( m3u + " contains no valid url" );
		}

		public override string GetTargetFilenameWithoutExtension() {
			return "twitch_" + Username + "_" + VideoId + "_" + VideoQuality + ( VideoInfo.status == "recorded" ? "" : "_" + VideoInfo.status );
		}
	}
}
