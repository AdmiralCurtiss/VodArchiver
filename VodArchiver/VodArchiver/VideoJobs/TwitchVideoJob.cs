using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using VodArchiver.VideoInfo;
using System.Xml;
using System.Threading;
using System.Diagnostics;

namespace VodArchiver.VideoJobs {
	public class TwitchVideoJob : TsVideoJob {
		string VideoQuality = "chunked";

		public TwitchVideoJob( string id, StatusUpdate.IStatusUpdate statusUpdater = null ) {
			JobStatus = VideoJobStatus.NotStarted;
			StatusUpdater = statusUpdater == null ? new StatusUpdate.NullStatusUpdate() : statusUpdater;
			VideoInfo = new GenericVideoInfo() { Service = StreamService.Twitch, VideoId = id };
		}

		public TwitchVideoJob( XmlNode node ) : base( node ) {
			VideoQuality = node.Attributes["videoQuality"].Value;
		}

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", "TwitchVideoJob" );
			XmlNode n = base.Serialize( document, node );
			n.AppendAttribute( document, "videoQuality", VideoQuality );
			return n;
		}

		public override async Task<(ResultType result, string[] urls)> GetFileUrlsOfVod( CancellationToken cancellationToken ) {
			VideoInfo = new TwitchVideoInfo( await TwitchV5.GetVideo( long.Parse( VideoInfo.VideoId ) ) );

			string folderpath;
			string[] filenames;
			while ( true ) {
				try {
					bool interactive = true;
					if ( interactive ) {
						Status = "";
						string tmp1 = Path.Combine( GetTempFolder(), GetTempFilenameWithoutExtension() + "_baseurl.txt" );
						string tmp2 = Path.Combine( GetTempFolder(), GetTempFilenameWithoutExtension() + "_tsnames.txt" );
						File.WriteAllText( tmp1, "get baseurl for m3u8 from https://www.twitch.tv/videos/" + VideoInfo.VideoId );
						File.WriteAllText( tmp2, "get actual m3u file from https://www.twitch.tv/videos/" + VideoInfo.VideoId );
						await Task.Delay( 200 );
						Process.Start( tmp1 );
						await Task.Delay( 200 );
						Process.Start( tmp2 );
						folderpath = TsVideoJob.GetFolder( GetM3U8PathFromM3U( await WaitForUserCopyM3U( tmp1 ), VideoQuality ) );
						filenames = TsVideoJob.GetFilenamesFromM3U8( await WaitForUserCopyM3U( tmp2 ) );
						await Task.Delay( 500 );
						File.Delete( tmp1 );
						File.Delete( tmp2 );
					} else {
						string m3u = await TwitchV5.GetVodM3U( long.Parse( VideoInfo.VideoId ) );
						string m3u8path = GetM3U8PathFromM3U( m3u, VideoQuality );
						folderpath = TsVideoJob.GetFolder( m3u8path );
						string m3u8 = await TwitchV5.Get( m3u8path );
						filenames = TsVideoJob.GetFilenamesFromM3U8( m3u8 );
					}
				} catch ( TwitchHttpException e ) {
					if ( e.StatusCode == System.Net.HttpStatusCode.NotFound && VideoInfo.VideoRecordingState == RecordingState.Live ) {
						// this can happen on streams that have just started, in this just wait a bit and retry
						try {
							await Task.Delay( 20000, cancellationToken );
						} catch ( TaskCanceledException ) {
							return (ResultType.Cancelled, null);
						}
						VideoInfo = new TwitchVideoInfo( await TwitchV5.GetVideo( long.Parse( VideoInfo.VideoId ) ) );
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
			return (ResultType.Success, urls.ToArray());
		}

		private async Task<string> WaitForUserCopyM3U( string tmp ) {
			while ( true ) {
				try {
					var lines = System.IO.File.ReadAllText( tmp );
					if ( lines.Contains( ".ts" ) || lines.Contains( "vod-secure.twitch.tv" ) ) {
						return lines;
					} else {
						await Task.Delay( 2000 );
					}
				} catch ( Exception ex ) { }
			}
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

		public override string GetTempFilenameWithoutExtension() {
			return "twitch_" + VideoInfo.Username + "_v" + VideoInfo.VideoId + "_" + VideoQuality;
		}

		public override string GetFinalFilenameWithoutExtension() {
			return "twitch_" + VideoInfo.Username + "_v" + VideoInfo.VideoId + "_" + Util.MakeIntercapsFilename( VideoInfo.VideoGame ?? "unknown" ) + "_" + Util.MakeIntercapsFilename( VideoInfo.VideoTitle ).Crop( 80 ) + "_" + VideoQuality;
		}
	}
}
