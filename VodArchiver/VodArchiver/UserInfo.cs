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
		HitboxRecordings
	}

	public class UserInfo : IEquatable<UserInfo>, IEqualityComparer<UserInfo>, IComparable<UserInfo> {
		public ServiceVideoCategoryType Service;
		public string Username;
		public bool AutoDownload;

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
			return Service + "/" + AutoDownload + "/" + Username;
		}

		public static UserInfo FromSerializableString( string s ) {
			UserInfo u = new UserInfo();

			string[] parts = s.Split( new char[] { '/' }, 3 );
			int partnum = 0;
			if ( !Enum.TryParse( parts[partnum++], out u.Service ) ) {
				throw new Exception( "Parsing ServiceVideoCategoryType from '" + parts[0] + "' failed." );
			}
			if ( parts.Length > 2 ) {
				u.AutoDownload = bool.Parse( parts[partnum++] );
			}
			u.Username = parts[partnum];
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

		public static string SerializationPath { get { return System.IO.Path.Combine( System.Windows.Forms.Application.LocalUserAppDataPath, "users.txt" ); } }

		public static void Load() {
			lock ( _KnownUsersLock ) {
				_KnownUsers = new SortedSet<UserInfo>();
				if ( System.IO.File.Exists( SerializationPath ) ) {
					string[] userList = System.IO.File.ReadAllLines( SerializationPath );
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
				System.IO.File.WriteAllLines( SerializationPath, userList );
			}
		}
	}
}
