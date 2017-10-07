using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using VodArchiver.VideoInfo;
using TwixelAPI;
using System.Xml;

namespace VodArchiver.VideoJobs {
	[Serializable]
	public class TwitchVideoJob : TsVideoJob {
		[NonSerialized]
		public Twixel TwitchAPI;
		string VideoQuality = "chunked";

		public TwitchVideoJob( Twixel api, string id, StatusUpdate.IStatusUpdate statusUpdater = null ) {
			JobStatus = VideoJobStatus.NotStarted;
			StatusUpdater = statusUpdater == null ? new StatusUpdate.NullStatusUpdate() : statusUpdater;
			VideoInfo = new GenericVideoInfo() { Service = StreamService.Twitch, VideoId = id };
			TwitchAPI = api;
		}

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", "TwitchVideoJob" );
			XmlNode n = base.Serialize( document, node );
			n.AppendAttribute( document, "videoQuality", VideoQuality );
			return n;
		}

		public override async Task<(bool success, string[] urls)> GetFileUrlsOfVod() {
			VideoInfo = new TwitchVideoInfo( await TwitchAPI.RetrieveVideo( VideoInfo.VideoId ) );

			string folderpath;
			string[] filenames;
			while ( true ) {
				try {
					string m3u = await TwitchAPI.RetrieveVodM3U( VideoInfo.VideoId );
					string m3u8path = GetM3U8PathFromM3U( m3u, VideoQuality );
					folderpath = TsVideoJob.GetFolder( m3u8path );
					string m3u8 = await Twixel.GetWebData( new Uri( m3u8path ) );
					filenames = TsVideoJob.GetFilenamesFromM3U8( m3u8 );
				} catch ( TwitchException e ) {
					if ( e.Status == 404 && VideoInfo.VideoRecordingState == RecordingState.Live ) {
						// this can happen on streams that have just started, in this just wait a bit and retry
						await Task.Delay( 20000 );
						VideoInfo = new TwitchVideoInfo( await TwitchAPI.RetrieveVideo( VideoInfo.VideoId ) );
						continue;
					} else {
						throw;
					}
				}
				break;
			}

			List<string> urls = new List<string>( filenames.Length );
			foreach ( var filename in filenames ) {
				urls.Add( folderpath + filename );
			}
			return (true, urls.ToArray());
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
			return "twitch_" + VideoInfo.Username + "_" + VideoInfo.VideoId + "_" + VideoQuality;
		}
	}
}
