using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using VodArchiver.VideoInfo;

namespace VodArchiver.UserInfo {
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


	public class GenericUserInfo : IUserInfo {
		public static int SerializeDataCount = 6;
		public ServiceVideoCategoryType Service;
		private bool _Persistable;
		public string Username;
		private bool _AutoDownload;
		public long? UserID;
		public string AdditionalOptions = "";
		private DateTime _LastRefreshedOn = Util.DateTimeFromUnixTime( 0 );

		public override ServiceVideoCategoryType Type => Service;
		public override string UserIdentifier => Username;
		public override bool Persistable { get => _Persistable; set => _Persistable = value; }
		public override bool AutoDownload { get => _AutoDownload; set => _AutoDownload = value; }
		public override DateTime LastRefreshedOn { get => _LastRefreshedOn; set => _LastRefreshedOn = value; }

		public override bool Equals( IUserInfo other ) {
			return other is GenericUserInfo && this.Service == ( other as GenericUserInfo ).Service && this.Username == ( other as GenericUserInfo ).Username;
		}

		public override bool Equals( IUserInfo x, IUserInfo y ) {
			return x.Equals( y );
		}

		public override int GetHashCode( IUserInfo obj ) {
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

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
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

		public static GenericUserInfo FromXmlNode( XmlNode node ) {
			GenericUserInfo u = new GenericUserInfo();
			u.Service = (ServiceVideoCategoryType)Enum.Parse( typeof( ServiceVideoCategoryType ), node.Attributes["_type"].Value );
			u.AutoDownload = node.Attributes["autoDownload"].Value == "true";
			u.UserID = node.Attributes["userID"].Value == "?" ? null : (long?)long.Parse( node.Attributes["userID"].Value );
			u.LastRefreshedOn = Util.DateTimeFromUnixTime( ulong.Parse( node.Attributes["lastRefreshedOn"].Value ) );
			u.AdditionalOptions = node.Attributes["preset"].Value;
			u.Username = node.Attributes["username"].Value;
			u.Persistable = true;
			return u;
		}

		public static GenericUserInfo FromSerializableString( string s, int splitcount ) {
			GenericUserInfo u = new GenericUserInfo();

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

		public override async Task<FetchReturnValue> Fetch( TwixelAPI.Twixel twitchApi, int offset, bool flat ) {
			return await Fetch( twitchApi, this, offset, flat );
		}

		private static async Task<FetchReturnValue> Fetch( TwixelAPI.Twixel twitchApi, GenericUserInfo userInfo, int offset, bool flat ) {
			List<IVideoInfo> videosToAdd = new List<IVideoInfo>();
			bool hasMore = true;
			long maxVideos = -1;
			int currentVideos = -1;
			bool forceReSave = false;

			switch ( userInfo.Type ) {
				case ServiceVideoCategoryType.TwitchRecordings:
				case ServiceVideoCategoryType.TwitchHighlights:
					if ( userInfo.UserID == null ) {
						userInfo.UserID = await twitchApi.GetUserId( userInfo.UserIdentifier, TwixelAPI.Twixel.APIVersion.v5 );
						if ( userInfo.UserID != null ) {
							forceReSave = true;
						}
					}
					TwixelAPI.Total<List<TwixelAPI.Video>> broadcasts = await twitchApi.RetrieveVideos(
						channel: userInfo.UserID == null ? userInfo.UserIdentifier : userInfo.UserID.ToString(),
						offset: offset, limit: 25, broadcasts: userInfo.Type == ServiceVideoCategoryType.TwitchRecordings, hls: false,
						version: userInfo.UserID == null ? TwixelAPI.Twixel.APIVersion.v3 : TwixelAPI.Twixel.APIVersion.v5
					);
					if ( broadcasts.total.HasValue ) {
						hasMore = offset + broadcasts.wrapped.Count < broadcasts.total;
						maxVideos = (long)broadcasts.total;
					} else {
						hasMore = broadcasts.wrapped.Count == 25;
					}
					foreach ( var v in broadcasts.wrapped ) {
						videosToAdd.Add( new TwitchVideoInfo( v, StreamService.Twitch ) );
						videosToAdd.Add( new TwitchVideoInfo( v, StreamService.TwitchChatReplay ) );
					}
					currentVideos = broadcasts.wrapped.Count;
					break;
				case ServiceVideoCategoryType.HitboxRecordings:
					(bool success, List<HitboxVideo> videos) = await Hitbox.RetrieveVideos( userInfo.UserIdentifier, offset: offset, limit: 100 );
					if ( success ) {
						hasMore = videos.Count == 100;
						foreach ( var v in videos ) {
							videosToAdd.Add( new HitboxVideoInfo( v ) );
						}
						currentVideos = videos.Count;
					}
					break;
				case ServiceVideoCategoryType.YoutubePlaylist:
					List<IVideoInfo> playlistVideos = await Youtube.RetrieveVideosFromPlaylist( userInfo.UserIdentifier, flat );
					hasMore = false;
					foreach ( var v in playlistVideos ) {
						videosToAdd.Add( v );
					}
					currentVideos = playlistVideos.Count;
					break;
				case ServiceVideoCategoryType.YoutubeChannel:
					List<IVideoInfo> channelVideos = await Youtube.RetrieveVideosFromChannel( userInfo.UserIdentifier, flat );
					hasMore = false;
					foreach ( var v in channelVideos ) {
						videosToAdd.Add( v );
					}
					currentVideos = channelVideos.Count;
					break;
				case ServiceVideoCategoryType.YoutubeUser:
					List<IVideoInfo> userVideos = await Youtube.RetrieveVideosFromUser( userInfo.UserIdentifier, flat );
					hasMore = false;
					foreach ( var v in userVideos ) {
						videosToAdd.Add( v );
					}
					currentVideos = userVideos.Count;
					break;
				case ServiceVideoCategoryType.RssFeed:
					List<IVideoInfo> rssFeedMedia = Rss.GetMediaFromFeed( userInfo.UserIdentifier );
					hasMore = false;
					foreach ( var m in rssFeedMedia ) {
						videosToAdd.Add( m );
					}
					currentVideos = rssFeedMedia.Count;
					break;
				case ServiceVideoCategoryType.FFMpegJob:
					List<IVideoInfo> reencodableFiles = await ReencodeFetcher.FetchReencodeableFiles( userInfo.UserIdentifier, userInfo.AdditionalOptions );
					hasMore = false;
					foreach ( var m in reencodableFiles ) {
						videosToAdd.Add( m );
					}
					currentVideos = reencodableFiles.Count;
					break;
				default:
					return new FetchReturnValue { Success = false, HasMore = false, TotalVideos = maxVideos, VideoCountThisFetch = 0, Videos = videosToAdd };
			}

			if ( videosToAdd.Count <= 0 ) {
				return new FetchReturnValue { Success = true, HasMore = false, TotalVideos = maxVideos, VideoCountThisFetch = 0, Videos = videosToAdd };
			}

			return new FetchReturnValue { Success = true, HasMore = hasMore, TotalVideos = maxVideos, VideoCountThisFetch = currentVideos, Videos = videosToAdd };
		}

		public override int GetHashCode() {
			return this.Username.GetHashCode() ^ this.Service.GetHashCode();
		}

		public override int CompareTo( IUserInfo other ) {
			if ( this.Type != other.Type ) {
				return this.Type.CompareTo( other.Type );
			}

			if ( !( other is GenericUserInfo ) ) {
				throw new Exception(); // if this happens something broke
			}

			return this.Username.CompareTo( ( other as GenericUserInfo ).Username );
		}
	}

	public class UserInfoPersister {
		private static object _KnownUsersLock = new object();
		private static SortedSet<IUserInfo> _KnownUsers = null;
		private static SortedSet<IUserInfo> KnownUsers {
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
				_KnownUsers = new SortedSet<IUserInfo>();
				if ( System.IO.File.Exists( Util.UserSerializationXmlPath ) ) {
					try {
						XmlDocument doc = new XmlDocument();
						using ( FileStream fs = System.IO.File.OpenRead( Util.UserSerializationXmlPath ) ) {
							doc.Load( fs );
						}
						foreach ( XmlNode node in doc.SelectNodes( "//root/UserInfo" ) ) {
							_KnownUsers.Add( IUserInfo.Deserialize( node ) );
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
						_KnownUsers.Add( GenericUserInfo.FromSerializableString( s, expectedAmountOfSections ) );
					}
					Save(); // convert to the new format
				}
			}
		}

		private static void SaveInternal( Stream fs, IEnumerable<IUserInfo> userInfos ) {
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

		public static bool Add( IUserInfo ui ) {
			lock ( _KnownUsersLock ) {
				return KnownUsers.Add( ui );
			}
		}

		public static bool AddOrUpdate( IUserInfo ui ) {
			lock ( _KnownUsersLock ) {
				return KnownUsers.AddOrUpdate( ui );
			}
		}

		public static List<IUserInfo> GetKnownUsers() {
			lock ( _KnownUsersLock ) {
				return new List<IUserInfo>( KnownUsers );
			}
		}
	}
}
