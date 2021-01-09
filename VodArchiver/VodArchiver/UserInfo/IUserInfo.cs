using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using VodArchiver.VideoInfo;

namespace VodArchiver.UserInfo {
	public struct FetchReturnValue { public bool Success; public bool HasMore; public long TotalVideos; public int VideoCountThisFetch; public List<IVideoInfo> Videos; }

	public abstract class IUserInfo : IEquatable<IUserInfo>, IEqualityComparer<IUserInfo>, IComparable<IUserInfo> {
		public abstract ServiceVideoCategoryType Type { get; }
		public abstract string UserIdentifier { get; }

		public abstract bool Persistable { get; set; }
		public abstract bool AutoDownload { get; set; }
		public abstract DateTime LastRefreshedOn { get; set; }

		public abstract XmlNode Serialize( XmlDocument document, XmlNode node );

		public static IUserInfo Deserialize( XmlNode node ) {
			ServiceVideoCategoryType type = (ServiceVideoCategoryType)Enum.Parse( typeof( ServiceVideoCategoryType ), node.Attributes["_type"].Value );
			switch ( type ) {
				case ServiceVideoCategoryType.TwitchRecordings:
				case ServiceVideoCategoryType.TwitchHighlights:
					return new TwitchUserInfo( node );
				case ServiceVideoCategoryType.HitboxRecordings:
					return new HitboxUserInfo( node );
				case ServiceVideoCategoryType.YoutubeUser:
					return new YoutubeUserUserInfo( node );
				case ServiceVideoCategoryType.YoutubeChannel:
					return new YoutubeChannelUserInfo( node );
				case ServiceVideoCategoryType.YoutubePlaylist:
					return new YoutubePlaylistUserInfo( node );
				case ServiceVideoCategoryType.RssFeed:
					return new RssFeedUserInfo( node );
				case ServiceVideoCategoryType.FFMpegJob:
					return new FFMpegJobUserInfo( node );
				case ServiceVideoCategoryType.ArchiveOrg:
					return new ArchiveOrgUserInfo(node);
				default:
					throw new Exception( "Unknown user info type: " + type );
			}
		}

		public abstract Task<FetchReturnValue> Fetch( int offset, bool flat );

		public virtual int CompareTo( IUserInfo other ) {
			if ( this.Type != other.Type ) {
				return this.Type.CompareTo( other.Type );
			}

			return this.UserIdentifier.CompareTo( other.UserIdentifier );
		}

		public virtual bool Equals( IUserInfo other ) {
			return this.Type == other.Type && this.UserIdentifier == other.UserIdentifier;
		}

		public virtual bool Equals( IUserInfo x, IUserInfo y ) {
			return x.Equals( y );
		}

		public override int GetHashCode() {
			return this.UserIdentifier.GetHashCode() ^ this.Type.GetHashCode();
		}

		public virtual int GetHashCode( IUserInfo obj ) {
			return obj.GetHashCode();
		}

		public override string ToString() {
			return Type + ": " + UserIdentifier;
		}
	}
}
