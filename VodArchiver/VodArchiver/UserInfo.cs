using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver {
	// terrible name but I can't think of a good one
	public enum ServiceVideoCategoryType {
		TwitchRecordings,
		TwitchHighlights,
		HitboxRecordings,
		YoutubeUser,
		YoutubeChannel,
		YoutubePlaylist,
	}

	public class UserInfo : IEquatable<UserInfo>, IEqualityComparer<UserInfo>, IComparable<UserInfo> {
		public ServiceVideoCategoryType Service;
		public bool Persistable;
		public string Username;
		public bool AutoDownload;
		public long? UserID;

		public bool Equals( UserInfo other ) {
			return this.Service == other.Service && this.Username == other.Username;
		}

		public bool Equals( UserInfo x, UserInfo y ) {
			return x.Equals( y );
		}

		public int GetHashCode( UserInfo obj ) {
			return obj.GetHashCode();
		}

		public override string ToString() {
			return Service + ": " + Username;
		}

		public string ToSerializableString() {
			String s = Service + "/" + AutoDownload + "/";
			s += UserID == null ? "?" : UserID.ToString();
			s += "/" + Username;
			return s;
		}

		public static UserInfo FromSerializableString( string s ) {
			UserInfo u = new UserInfo();

			string[] parts = s.Split( new char[] { '/' }, 4 );
			int partnum = 0;
			if ( !Enum.TryParse( parts[partnum++], out u.Service ) ) {
				throw new Exception( "Parsing ServiceVideoCategoryType from '" + parts[0] + "' failed." );
			}
			if ( parts.Length > 2 ) {
				u.AutoDownload = bool.Parse( parts[partnum++] );
			}
			if ( parts.Length > 3 ) {
				string uid = parts[partnum++];
				u.UserID = uid == "?" ? null : (long?)long.Parse( uid );
			}
			u.Username = parts[partnum];
			u.Persistable = true;
			return u;
		}

		public override int GetHashCode() {
			return this.Username.GetHashCode() ^ this.Service.GetHashCode();
		}

		public int CompareTo( UserInfo other ) {
			if ( this.Service != other.Service ) {
				return this.Service.CompareTo( other.Service );
			}

			return this.Username.CompareTo( other.Username );
		}
	}

	public class UserInfoPersister {
		private static object _KnownUsersLock = new object();
		private static SortedSet<UserInfo> _KnownUsers = null;
		public static SortedSet<UserInfo> KnownUsers {
			get {
				if ( _KnownUsers == null ) {
					Load();
				}
				return _KnownUsers;
			}
		}

		public static void Load() {
			lock ( _KnownUsersLock ) {
				_KnownUsers = new SortedSet<UserInfo>();
				if ( System.IO.File.Exists( Util.UserSerializationPath ) ) {
					string[] userList = System.IO.File.ReadAllLines( Util.UserSerializationPath );
					foreach ( var s in userList ) {
						_KnownUsers.Add( UserInfo.FromSerializableString( s ) );
					}
				}
			}
		}

		public static void Save() {
			lock ( _KnownUsersLock ) {
				List<string> userList = new List<string>( KnownUsers.Count );
				foreach ( var u in KnownUsers ) {
					userList.Add( u.ToSerializableString() );
				}
				System.IO.File.WriteAllLines( Util.UserSerializationPath, userList );
			}
		}
	}
}
