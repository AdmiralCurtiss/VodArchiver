using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.ServiceModel.Syndication;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using VodArchiver.VideoInfo;

namespace VodArchiver {
	class Rss {
		private static string GetStringFromSyndicationContent( SyndicationContent content ) {
			TextSyndicationContent tx = content as TextSyndicationContent;
			UrlSyndicationContent ur = content as UrlSyndicationContent;
			XmlSyndicationContent xm = content as XmlSyndicationContent;
			string text = "";
			if ( tx != null ) {
				text = tx.Text;
			} else if ( ur != null ) {
				text = new KeepAliveWebClient().DownloadString( ur.Url );
			} else if ( xm != null ) {
				// is this right...?
				text = GetStringViaXmlStream( ( x => content.WriteTo( x, "", "" ) ) );
			}

			return text;
		}

		private static string GetStringFromSyndicationElementExtension( SyndicationElementExtension extension ) {
			return GetStringViaXmlStream( ( x => extension.WriteTo( x ) ) );
			
		}

		private static string GetStringViaXmlStream( Action<XmlTextWriter> func ) {
			using ( MemoryStream ms = new MemoryStream() )
			using ( StreamWriter sw = new System.IO.StreamWriter( ms ) )
			using ( XmlTextWriter tmp = new XmlTextWriter( sw ) ) {
				func.Invoke( tmp );
				sw.Flush();
				ms.Position = 0;
				using ( StreamReader sr = new StreamReader( ms ) ) {
					return sr.ReadToEnd();
				}
			}
		}

		public static List<IVideoInfo> GetMediaFromFeed( string url ) {
			SyndicationFeed feed;
			using ( XmlReader reader = XmlReader.Create( url ) ) {
				feed = SyndicationFeed.Load( reader );
			}

			List<IVideoInfo> media = new List<IVideoInfo>();
			foreach ( SyndicationItem item in feed.Items ) {
				SyndicationLink link = item.Links.Where( x => x.MediaType != null ).FirstOrDefault();
				if ( link == null ) {
					continue;
				}

				string description = GetStringFromSyndicationContent( item.Summary );
				if ( String.IsNullOrWhiteSpace( description ) ) {
					description = GetStringFromSyndicationContent( item.Content );
				}

				var durationElement = item.ElementExtensions.Where( x => x.OuterName == "duration" ).FirstOrDefault();
				TimeSpan duration = TimeSpan.Zero;
				try {
					if ( durationElement != null ) {
						string durationString = GetStringFromSyndicationElementExtension( durationElement );
						using ( XmlReader durationReader = XmlReader.Create( new StringReader( durationString ) ) ) {
							durationReader.Read();
							string d = durationReader.ReadElementContentAsString();
							duration = TimeSpan.Parse( d );
						}
					}
				} catch ( Exception ) {
				}

				GenericVideoInfo info = new GenericVideoInfo {
					Service = StreamService.RawUrl,
					VideoTitle = item.Title.Text,
					VideoGame = description,
					VideoTimestamp = item.PublishDate.UtcDateTime,
					VideoType = VideoFileType.Unknown,
					VideoRecordingState = RecordingState.Recorded,
					Username = feed.Title.Text,
					VideoId = link.Uri.AbsoluteUri,
					VideoLength = duration
				};
				media.Add( info );
			}
			return media;
		}
	}
}
