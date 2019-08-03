using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using VodArchiver.VideoInfo;

namespace VodArchiver.UserInfo {
	public class YoutubePlaylistUserInfo : IUserInfo {
		public override ServiceVideoCategoryType Type => ServiceVideoCategoryType.YoutubePlaylist;
		public override string UserIdentifier => Playlist;
		public override bool Persistable { get => _Persistable; set => _Persistable = value; }
		public override bool AutoDownload { get => _AutoDownload; set => _AutoDownload = value; }
		public override DateTime LastRefreshedOn { get => _LastRefreshedOn; set => _LastRefreshedOn = value; }

		private bool _Persistable;
		private bool _AutoDownload;
		private DateTime _LastRefreshedOn = Util.DateTimeFromUnixTime( 0 );

		private string Playlist;

		public YoutubePlaylistUserInfo( string playlist ) {
			if ( playlist.StartsWith( "http://" ) || playlist.StartsWith( "https://" ) ) {
				Playlist = Util.GetParameterFromUri( new Uri( playlist ), "list" );
			} else {
				Playlist = playlist;
			}
			_Persistable = false;
			_AutoDownload = false;
		}

		public YoutubePlaylistUserInfo( XmlNode node ) {
			_AutoDownload = node.Attributes["autoDownload"].Value == "true";
			_LastRefreshedOn = Util.DateTimeFromUnixTime( ulong.Parse( node.Attributes["lastRefreshedOn"].Value ) );
			Playlist = node.Attributes["playlist"].Value;
			_Persistable = true;
		}

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", Type.ToString() );
			node.AppendAttribute( document, "autoDownload", AutoDownload ? "true" : "false" );
			node.AppendAttribute( document, "lastRefreshedOn", LastRefreshedOn.DateTimeToUnixTime().ToString() );
			node.AppendAttribute( document, "playlist", Playlist.ToString() );
			return node;
		}

		public override async Task<FetchReturnValue> Fetch( int offset, bool flat ) {
			List<IVideoInfo> videosToAdd = new List<IVideoInfo>();
			bool hasMore = true;
			long maxVideos = -1;
			int currentVideos = -1;

			List<IVideoInfo> playlistVideos = await Youtube.RetrieveVideosFromPlaylist( Playlist, flat );
			hasMore = false;
			foreach ( var v in playlistVideos ) {
				videosToAdd.Add( v );
			}
			currentVideos = playlistVideos.Count;

			if ( videosToAdd.Count <= 0 ) {
				return new FetchReturnValue { Success = true, HasMore = false, TotalVideos = maxVideos, VideoCountThisFetch = 0, Videos = videosToAdd };
			}

			return new FetchReturnValue { Success = true, HasMore = hasMore, TotalVideos = maxVideos, VideoCountThisFetch = currentVideos, Videos = videosToAdd };
		}
	}
}
