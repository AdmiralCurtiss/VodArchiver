using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using VodArchiver.VideoInfo;

namespace VodArchiver.UserInfo {
	public class ArchiveOrgUserInfo : IUserInfo {
		public override ServiceVideoCategoryType Type => ServiceVideoCategoryType.FFMpegJob;
		public override string UserIdentifier => Identifier;
		public override bool Persistable { get => _Persistable; set => _Persistable = value; }
		public override bool AutoDownload { get => _AutoDownload; set => _AutoDownload = value; }
		public override DateTime LastRefreshedOn { get => _LastRefreshedOn; set => _LastRefreshedOn = value; }

		private bool _Persistable;
		private bool _AutoDownload;
		private DateTime _LastRefreshedOn = Util.DateTimeFromUnixTime(0);

		private string Identifier;

		public ArchiveOrgUserInfo(string url) {
			Identifier = url;
			_Persistable = false;
			_AutoDownload = false;
		}

		public ArchiveOrgUserInfo(XmlNode node) {
			_AutoDownload = node.Attributes["autoDownload"].Value == "true";
			_LastRefreshedOn = Util.DateTimeFromUnixTime(ulong.Parse(node.Attributes["lastRefreshedOn"].Value));
			Identifier = node.Attributes["identifier"].Value;
			_Persistable = true;
		}

		public override XmlNode Serialize(XmlDocument document, XmlNode node) {
			node.AppendAttribute(document, "_type", Type.ToString());
			node.AppendAttribute(document, "autoDownload", AutoDownload ? "true" : "false");
			node.AppendAttribute(document, "lastRefreshedOn", LastRefreshedOn.DateTimeToUnixTime().ToString());
			node.AppendAttribute(document, "identifier", Identifier);
			return node;
		}

		public override async Task<FetchReturnValue> Fetch(int offset, bool flat) {
			List<IVideoInfo> videosToAdd = new List<IVideoInfo>();
			bool hasMore = true;
			long maxVideos = -1;
			int currentVideos = -1;

			List<IVideoInfo> rssFeedMedia = await ArchiveOrg.GetFilesFromUrl(Identifier);
			hasMore = false;
			foreach (var m in rssFeedMedia) {
				videosToAdd.Add(m);
			}
			currentVideos = rssFeedMedia.Count;

			if (videosToAdd.Count <= 0) {
				return new FetchReturnValue { Success = true, HasMore = false, TotalVideos = maxVideos, VideoCountThisFetch = 0, Videos = videosToAdd };
			}

			return new FetchReturnValue { Success = true, HasMore = hasMore, TotalVideos = maxVideos, VideoCountThisFetch = currentVideos, Videos = videosToAdd };
		}
	}
}
