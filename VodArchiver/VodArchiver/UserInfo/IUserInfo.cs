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
				default:
					return GenericUserInfo.FromXmlNode( node );
					//throw new Exception( "Unknown user info type: " + type );
			}
		}

		public abstract Task<FetchReturnValue> Fetch( TwixelAPI.Twixel twitchApi, int offset, bool flat );

		public abstract int CompareTo( IUserInfo other );
		public abstract bool Equals( IUserInfo other );
		public abstract bool Equals( IUserInfo x, IUserInfo y );
		public abstract int GetHashCode( IUserInfo obj );
	}
}
