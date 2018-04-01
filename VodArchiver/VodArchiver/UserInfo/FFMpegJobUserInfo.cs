using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using VodArchiver.VideoInfo;

namespace VodArchiver.UserInfo {
	public class FFMpegJobUserInfo : IUserInfo {
		public override ServiceVideoCategoryType Type => ServiceVideoCategoryType.FFMpegJob;
		public override string UserIdentifier => Path;
		public override bool Persistable { get => _Persistable; set => _Persistable = value; }
		public override bool AutoDownload { get => _AutoDownload; set => _AutoDownload = value; }
		public override DateTime LastRefreshedOn { get => _LastRefreshedOn; set => _LastRefreshedOn = value; }

		private bool _Persistable;
		private bool _AutoDownload;
		private DateTime _LastRefreshedOn = Util.DateTimeFromUnixTime( 0 );

		private string Path;
		private string Preset;

		public FFMpegJobUserInfo( XmlNode node ) {
			_AutoDownload = node.Attributes["autoDownload"].Value == "true";
			_LastRefreshedOn = Util.DateTimeFromUnixTime( ulong.Parse( node.Attributes["lastRefreshedOn"].Value ) );
			Path = node.Attributes["path"].Value;
			Preset = node.Attributes["preset"].Value;
			_Persistable = true;
		}

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", Type.ToString() );
			node.AppendAttribute( document, "autoDownload", AutoDownload ? "true" : "false" );
			node.AppendAttribute( document, "lastRefreshedOn", LastRefreshedOn.DateTimeToUnixTime().ToString() );
			node.AppendAttribute( document, "path", Path );
			node.AppendAttribute( document, "preset", Preset );
			return node;
		}

		public override async Task<FetchReturnValue> Fetch( TwixelAPI.Twixel twitchApi, int offset, bool flat ) {
			List<IVideoInfo> videosToAdd = new List<IVideoInfo>();
			bool hasMore = true;
			long maxVideos = -1;
			int currentVideos = -1;

			List<IVideoInfo> reencodableFiles = await ReencodeFetcher.FetchReencodeableFiles( Path, Preset );
			hasMore = false;
			foreach ( var m in reencodableFiles ) {
				videosToAdd.Add( m );
			}
			currentVideos = reencodableFiles.Count;

			if ( videosToAdd.Count <= 0 ) {
				return new FetchReturnValue { Success = true, HasMore = false, TotalVideos = maxVideos, VideoCountThisFetch = 0, Videos = videosToAdd };
			}

			return new FetchReturnValue { Success = true, HasMore = hasMore, TotalVideos = maxVideos, VideoCountThisFetch = currentVideos, Videos = videosToAdd };
		}
	}
}
