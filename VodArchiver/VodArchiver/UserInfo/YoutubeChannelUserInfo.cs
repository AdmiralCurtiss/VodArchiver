﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using VodArchiver.VideoInfo;

namespace VodArchiver.UserInfo {
	public class YoutubeChannelUserInfo : IUserInfo {
		public override ServiceVideoCategoryType Type => ServiceVideoCategoryType.YoutubeChannel;
		public override string UserIdentifier => Channel;
		public override bool Persistable { get => _Persistable; set => _Persistable = value; }
		public override bool AutoDownload { get => _AutoDownload; set => _AutoDownload = value; }
		public override DateTime LastRefreshedOn { get => _LastRefreshedOn; set => _LastRefreshedOn = value; }

		private bool _Persistable;
		private bool _AutoDownload;
		private DateTime _LastRefreshedOn = Util.DateTimeFromUnixTime( 0 );

		private string Channel;
		public string Comment;

		public YoutubeChannelUserInfo( string channel ) {
			Channel = channel;
			_Persistable = false;
			_AutoDownload = false;
		}

		public YoutubeChannelUserInfo( XmlNode node ) {
			_AutoDownload = node.Attributes["autoDownload"].Value == "true";
			_LastRefreshedOn = Util.DateTimeFromUnixTime( ulong.Parse( node.Attributes["lastRefreshedOn"].Value ) );
			Channel = node.Attributes["channel"].Value;
			Comment = node?.Attributes["comment"]?.Value;
			_Persistable = true;
		}

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", Type.ToString() );
			node.AppendAttribute( document, "autoDownload", AutoDownload ? "true" : "false" );
			node.AppendAttribute( document, "lastRefreshedOn", LastRefreshedOn.DateTimeToUnixTime().ToString() );
			node.AppendAttribute( document, "channel", Channel.ToString() );
			if (Comment != null) {
				node.AppendAttribute(document, "comment", Comment);
			}
			return node;
		}

		public override async Task<FetchReturnValue> Fetch( int offset, bool flat ) {
			List<IVideoInfo> videosToAdd = new List<IVideoInfo>();
			bool hasMore = true;
			long maxVideos = -1;
			int currentVideos = -1;

			List<IVideoInfo> channelVideos = await Youtube.RetrieveVideosFromChannel( Channel, flat, Comment );
			hasMore = false;
			foreach ( var v in channelVideos ) {
				videosToAdd.Add( v );
			}
			currentVideos = channelVideos.Count;

			if ( videosToAdd.Count <= 0 ) {
				return new FetchReturnValue { Success = true, HasMore = false, TotalVideos = maxVideos, VideoCountThisFetch = 0, Videos = videosToAdd };
			}

			return new FetchReturnValue { Success = true, HasMore = hasMore, TotalVideos = maxVideos, VideoCountThisFetch = currentVideos, Videos = videosToAdd };
		}

		public override string ToString() {
			if (Comment != null) {
				return Type + ": [" + Comment + "] " + UserIdentifier;
			}
			return base.ToString();
		}
	}
}
