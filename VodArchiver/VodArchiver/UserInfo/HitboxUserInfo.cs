using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using VodArchiver.VideoInfo;

namespace VodArchiver.UserInfo {
	public class HitboxUserInfo : IUserInfo {
		public override ServiceVideoCategoryType Type => ServiceVideoCategoryType.HitboxRecordings;
		public override string UserIdentifier => Username;
		public override bool Persistable { get => _Persistable; set => _Persistable = value; }
		public override bool AutoDownload { get => _AutoDownload; set => _AutoDownload = value; }
		public override DateTime LastRefreshedOn { get => _LastRefreshedOn; set => _LastRefreshedOn = value; }

		private bool _Persistable;
		private bool _AutoDownload;
		private DateTime _LastRefreshedOn = Util.DateTimeFromUnixTime( 0 );

		private string Username;

		public HitboxUserInfo( XmlNode node ) {
			_AutoDownload = node.Attributes["autoDownload"].Value == "true";
			_LastRefreshedOn = Util.DateTimeFromUnixTime( ulong.Parse( node.Attributes["lastRefreshedOn"].Value ) );
			Username = node.Attributes["username"].Value;
			_Persistable = true;
		}

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", Type.ToString() );
			node.AppendAttribute( document, "autoDownload", AutoDownload ? "true" : "false" );
			node.AppendAttribute( document, "lastRefreshedOn", LastRefreshedOn.DateTimeToUnixTime().ToString() );
			node.AppendAttribute( document, "username", Username.ToString() );
			return node;
		}

		public override async Task<FetchReturnValue> Fetch( TwixelAPI.Twixel twitchApi, int offset, bool flat ) {
			List<IVideoInfo> videosToAdd = new List<IVideoInfo>();
			bool hasMore = true;
			long maxVideos = -1;
			int currentVideos = -1;

			(bool success, List<HitboxVideo> videos) = await Hitbox.RetrieveVideos( Username, offset: offset, limit: 100 );
			if ( success ) {
				hasMore = videos.Count == 100;
				foreach ( var v in videos ) {
					videosToAdd.Add( new HitboxVideoInfo( v ) );
				}
				currentVideos = videos.Count;
			}

			if ( videosToAdd.Count <= 0 ) {
				return new FetchReturnValue { Success = true, HasMore = false, TotalVideos = maxVideos, VideoCountThisFetch = 0, Videos = videosToAdd };
			}

			return new FetchReturnValue { Success = true, HasMore = hasMore, TotalVideos = maxVideos, VideoCountThisFetch = currentVideos, Videos = videosToAdd };
		}
	}
}
