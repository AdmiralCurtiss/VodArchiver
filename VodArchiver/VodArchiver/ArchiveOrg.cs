using Argotic.Syndication;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using System.Xml.XPath;
using VodArchiver.VideoInfo;

namespace VodArchiver {
	class ArchiveOrg {
		public static async Task<List<IVideoInfo>> GetFilesFromUrl(string identifier) {
			string url = $"https://archive.org/download/{identifier}/{identifier}_files.xml";
			XmlReaderSettings settings = new XmlReaderSettings();
			settings.Async = true;

			var vi = new List<IVideoInfo>();

			XmlDocument doc = new XmlDocument();
			using (XmlReader reader = XmlReader.Create(url, settings)) {
				XPathDocument xml = new XPathDocument(reader);
				var nav = xml.CreateNavigator();
				var files = nav.SelectSingleNode("files");
				if (files.MoveToFirstChild()) {
					do {
						if (files.Name == "file" && files.HasAttributes) {
							string filename = files.GetAttribute("name", "");
							string source = files.GetAttribute("source", "");
							DateTime datetime = Util.DateTimeFromUnixTime(0);
							try {
								long timestamp = files.SelectSingleNode("mtime").ValueAsLong;
								datetime = Util.DateTimeFromUnixTime(timestamp >= 0 ? (ulong)timestamp : 0);
							} catch (Exception) { }

							var v = new GenericVideoInfo();
							v.Service = StreamService.RawUrl;
							v.Username = identifier;
							v.VideoId = $"https://archive.org/download/{identifier}/{filename}";
							v.VideoTitle = filename;
							v.VideoGame = source;
							v.VideoTimestamp = datetime;
							v.VideoLength = TimeSpan.Zero;
							v.VideoRecordingState = RecordingState.Unknown;
							v.VideoType = VideoFileType.Unknown;
							vi.Add(v);
						}
					} while (files.MoveToNext());
				}
			}

			return vi;
		}
	}
}
