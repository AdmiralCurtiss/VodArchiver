using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

namespace VodArchiver {
	// terrible name but I can't think of a good one
	public enum ServiceVideoCategoryType {
		TwitchRecordings,
		TwitchHighlights,
		HitboxRecordings,
		YoutubeUser,
		YoutubeChannel,
		YoutubePlaylist,
		RssFeed,
		FFMpegJob,
	}

	public static class ServiceVideoCategoryGroups {
		public static List<List<ServiceVideoCategoryType>> Groups {
			get {
				return new List<List<ServiceVideoCategoryType>> {
					new List<ServiceVideoCategoryType> {
						ServiceVideoCategoryType.TwitchRecordings,
						ServiceVideoCategoryType.TwitchHighlights,
					},
					new List<ServiceVideoCategoryType> {
						ServiceVideoCategoryType.HitboxRecordings,
					},
					new List<ServiceVideoCategoryType> {
						ServiceVideoCategoryType.YoutubeUser,
						ServiceVideoCategoryType.YoutubeChannel,
						ServiceVideoCategoryType.YoutubePlaylist,
					},
					new List<ServiceVideoCategoryType> {
						ServiceVideoCategoryType.RssFeed,
					},
					new List<ServiceVideoCategoryType> {
						ServiceVideoCategoryType.FFMpegJob,
					},
				};
			}
		}
	}


	public class UserInfo : IEquatable<UserInfo>, IEqualityComparer<UserInfo>, IComparable<UserInfo> {
		public static int SerializeDataCount = 6;
		public ServiceVideoCategoryType Service;
		public bool Persistable;
		public string Username;
		public bool AutoDownload;
		public long? UserID;
		public string AdditionalOptions = "";
		public DateTime LastRefreshedOn = Util.DateTimeFromUnixTime( 0 );

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
			s += "/";
			s += LastRefreshedOn == null ? "?" : LastRefreshedOn.DateTimeToUnixTime().ToString();
			s += "/" + AdditionalOptions;
			s += "/" + Username;
			return s;
		}

		public XmlNode Serialize( XmlDocument document, XmlNode node ) {
			// this looks somewhat stupid but is to ensure that a type-specific deserializer can find the attributes at logical names instead of having to check legacy stuff
			node.AppendAttribute( document, "_type", Service.ToString() );
			node.AppendAttribute( document, "autoDownload", AutoDownload ? "true" : "false" );
			node.AppendAttribute( document, "userID", UserID == null ? "?" : UserID.ToString() );
			node.AppendAttribute( document, "lastRefreshedOn", LastRefreshedOn.DateTimeToUnixTime().ToString() );
			node.AppendAttribute( document, "preset", AdditionalOptions );
			node.AppendAttribute( document, "username", Username.ToString() );
			node.AppendAttribute( document, "channel", Username.ToString() );
			node.AppendAttribute( document, "playlist", Username.ToString() );
			node.AppendAttribute( document, "url", Username.ToString() );
			node.AppendAttribute( document, "path", Username.ToString() );
			return node;
		}

		public static UserInfo FromXmlNode( XmlNode node ) {
			UserInfo u = new UserInfo();
			u.Service = (ServiceVideoCategoryType)Enum.Parse( typeof( ServiceVideoCategoryType ), node.Attributes["_type"].Value );
			u.AutoDownload = node.Attributes["autoDownload"].Value == "true";
			u.UserID = node.Attributes["userID"].Value == "?" ? null : (long?)long.Parse( node.Attributes["userID"].Value );
			u.LastRefreshedOn = Util.DateTimeFromUnixTime( ulong.Parse( node.Attributes["lastRefreshedOn"].Value ) );
			u.AdditionalOptions = node.Attributes["preset"].Value;
			u.Username = node.Attributes["username"].Value;
			u.Persistable = true;
			return u;
		}

		public static UserInfo FromSerializableString( string s, int splitcount ) {
			UserInfo u = new UserInfo();

			string[] parts = s.Split( new char[] { '/' }, splitcount );
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
			if ( parts.Length > 4 ) {
				string tmp = parts[partnum++];
				u.LastRefreshedOn = Util.DateTimeFromUnixTime( tmp == "?" ? 0 : ulong.Parse( tmp ) );
			}
			if ( parts.Length > 5 ) {
				string tmp = parts[partnum++];
				u.AdditionalOptions = tmp;
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
		private static SortedSet<UserInfo> KnownUsers {
			get {
				lock ( _KnownUsersLock ) {
					if ( _KnownUsers == null ) {
						Load();
					}
					return _KnownUsers;
				}
			}
		}

		public static void Load() {
			lock ( _KnownUsersLock ) {
				_KnownUsers = new SortedSet<UserInfo>();
				if ( System.IO.File.Exists( Util.UserSerializationXmlPath ) ) {
					try {
						XmlDocument doc = new XmlDocument();
						using ( FileStream fs = System.IO.File.OpenRead( Util.UserSerializationXmlPath ) ) {
							doc.Load( fs );
						}
						foreach ( XmlNode node in doc.SelectNodes( "//root/UserInfo" ) ) {
							_KnownUsers.Add( UserInfo.FromXmlNode( node ) );
						}
					} catch ( System.Runtime.Serialization.SerializationException ) { } catch ( FileNotFoundException ) { }
				} else if ( System.IO.File.Exists( Util.UserSerializationPath ) ) {
					string[] userList = System.IO.File.ReadAllLines( Util.UserSerializationPath );
					int expectedAmountOfSections = 4; // if no #datacount label in file assume 4, that was the amount at the time the label was implemented
					foreach ( var s in userList ) {
						if ( s.StartsWith( "#" ) ) {
							if ( s.StartsWith( "#datacount" ) ) {
								int tmp;
								if ( Int32.TryParse( s.Substring( "#datacount".Length ).Trim(), out tmp ) ) {
									expectedAmountOfSections = tmp;
								}
							}
							continue;
						}
						_KnownUsers.Add( UserInfo.FromSerializableString( s, expectedAmountOfSections ) );
					}
					Save(); // convert to the new format
				}
			}
		}

		private static void SaveInternal( Stream fs, IEnumerable<UserInfo> userInfos ) {
			XmlDocument document = new XmlDocument();
			XmlNode node = document.CreateElement( "root" );
			foreach ( var userInfo in userInfos ) {
				node.AppendChild( userInfo.Serialize( document, document.CreateElement( "UserInfo" ) ) );
			}
			document.AppendChild( node );
			using ( StreamWriter sw = new StreamWriter( fs, new UTF8Encoding( false, true ), 1024 * 1024, true ) ) {
				document.Save( XmlWriter.Create( sw, new XmlWriterSettings() { Indent = true } ) );
			}
		}

		public static void Save() {
			lock ( _KnownUsersLock ) {
				using ( MemoryStream memoryStream = new MemoryStream() ) {
					SaveInternal( memoryStream, KnownUsers );
					long count = memoryStream.Position;
					memoryStream.Position = 0;
					using ( FileStream fs = File.Create( Util.UserSerializationXmlTempPath ) ) {
						Util.CopyStream( memoryStream, fs, count );
						fs.Close();
					}
					if ( File.Exists( Util.UserSerializationXmlPath ) ) {
						System.Threading.Thread.Sleep( 100 );
						File.Delete( Util.UserSerializationXmlPath );
						System.Threading.Thread.Sleep( 100 );
					}
					File.Move( Util.UserSerializationXmlTempPath, Util.UserSerializationXmlPath );
					System.Threading.Thread.Sleep( 100 );
				}
			}
		}

		public static bool Add( UserInfo ui ) {
			lock ( _KnownUsersLock ) {
				return KnownUsers.Add( ui );
			}
		}

		public static bool AddOrUpdate( UserInfo ui ) {
			lock ( _KnownUsersLock ) {
				return KnownUsers.AddOrUpdate( ui );
			}
		}

		public static List<UserInfo> GetKnownUsers() {
			lock ( _KnownUsersLock ) {
				return new List<UserInfo>( KnownUsers );
			}
		}
	}
}
