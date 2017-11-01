using Argotic.Syndication;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using VodArchiver.VideoInfo;

namespace VodArchiver {
	class Rss {
		public static List<IVideoInfo> GetMediaFromFeed( string url ) {
			RssFeed feed = new RssFeed();
			using ( XmlReader reader = XmlReader.Create( url ) ) {
				feed.Load( reader );
			}

			List<IVideoInfo> media = new List<IVideoInfo>();
			foreach ( RssItem item in feed.Channel.Items ) {
				RssEnclosure enclosure = item.Enclosures.Where( x => x.ContentType != null ).FirstOrDefault();
				if ( enclosure == null ) {
					continue;
				}

				var durationElement = item.FindExtension( x => x is Argotic.Extensions.Core.ITunesSyndicationExtension && ( (Argotic.Extensions.Core.ITunesSyndicationExtension)x ).Context.Duration != null );
				TimeSpan duration = durationElement != null ? ( (Argotic.Extensions.Core.ITunesSyndicationExtension)durationElement ).Context.Duration : TimeSpan.Zero;

				GenericVideoInfo info = new GenericVideoInfo {
					Service = StreamService.RawUrl,
					VideoTitle = item.Title,
					VideoGame = item.Description,
					VideoTimestamp = item.PublicationDate,
					VideoType = VideoFileType.Unknown,
					VideoRecordingState = RecordingState.Recorded,
					Username = feed.Channel.Title,
					VideoId = enclosure.Url.AbsoluteUri,
					VideoLength = duration
				};
				media.Add( info );
			}
			return media;
		}
	}
}
