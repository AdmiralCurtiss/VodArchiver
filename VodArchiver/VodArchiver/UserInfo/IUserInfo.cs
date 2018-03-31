using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

namespace VodArchiver.UserInfo {
	public abstract class IUserInfo : IEquatable<IUserInfo>, IEqualityComparer<IUserInfo>, IComparable<IUserInfo> {
		public abstract ServiceVideoCategoryType Type { get; }

		public static IUserInfo Deserialize( XmlNode node ) {
			ServiceVideoCategoryType type = (ServiceVideoCategoryType)Enum.Parse( typeof( ServiceVideoCategoryType ), node.Attributes["_type"].Value );
			switch ( type ) {
				default:
					return GenericUserInfo.FromXmlNode( node );
					//throw new Exception( "Unknown user info type: " + type );
			}
		}

		public abstract int CompareTo( IUserInfo other );
		public abstract bool Equals( IUserInfo other );
		public abstract bool Equals( IUserInfo x, IUserInfo y );
		public abstract int GetHashCode( IUserInfo obj );
	}
}
