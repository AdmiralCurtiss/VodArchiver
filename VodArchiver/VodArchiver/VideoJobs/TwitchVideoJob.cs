using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using VodArchiver.VideoInfo;
using System.Xml;
using System.Threading;

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
					string m3u = await TwitchV5.GetVodM3U( long.Parse( VideoInfo.VideoId ) );
					string m3u8path = GetM3U8PathFromM3U( m3u, VideoQuality );
					folderpath = TsVideoJob.GetFolder( m3u8path );
					string m3u8 = await TwitchV5.Get( m3u8path );
					filenames = TsVideoJob.GetFilenamesFromM3U8( m3u8 );
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
			return "twitch_" + VideoInfo.Username + "_" + VideoInfo.VideoId + "_" + VideoQuality;
		}

		public override string GetFinalFilenameWithoutExtension() {
			return "twitch_" + VideoInfo.Username + "_" + VideoInfo.VideoId + "_" + CleanGamenameForFilename( VideoInfo.VideoGame ) + "_" + VideoQuality;
		}

		private static string ToUpperFirstChar( string s ) {
			if ( s.Length > 0 ) {
				bool maybeRomanNumeral = s.All( c => "IVX".Contains( c ) );
				if ( maybeRomanNumeral ) {
					return s;
				} else {
					return s.Substring( 0, 1 ).ToUpperInvariant() + s.Substring( 1 ).ToLowerInvariant();
				}
			} else {
				return s;
			}
		}
		private static string CleanGamenameForFilename( string gamename ) {
			string safename = Util.MakeStringFileSystemSafeBaseName( gamename ).Replace( "-", "_" );
			return string.Join( "", safename.Split( new char[] { '_' }, StringSplitOptions.RemoveEmptyEntries ).Select( x => ToUpperFirstChar( x ) ) );
		}
	}
}
