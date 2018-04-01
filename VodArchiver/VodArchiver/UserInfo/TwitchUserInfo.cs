﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using VodArchiver.VideoInfo;

namespace VodArchiver.UserInfo {
	public class TwitchUserInfo : IUserInfo {
		public override ServiceVideoCategoryType Type => Highlights ? ServiceVideoCategoryType.TwitchHighlights : ServiceVideoCategoryType.TwitchRecordings;
		public override string UserIdentifier => Username;
		public override bool Persistable { get => _Persistable; set => _Persistable = value; }
		public override bool AutoDownload { get => _AutoDownload; set => _AutoDownload = value; }
		public override DateTime LastRefreshedOn { get => _LastRefreshedOn; set => _LastRefreshedOn = value; }

		private bool _Persistable;
		private bool _AutoDownload;
		private DateTime _LastRefreshedOn = Util.DateTimeFromUnixTime( 0 );

		private string Username;
		private long? UserID;
		private bool Highlights;

		public TwitchUserInfo( XmlNode node ) {
			ServiceVideoCategoryType t = ( ServiceVideoCategoryType)Enum.Parse( typeof( ServiceVideoCategoryType ), node.Attributes["_type"].Value );
			if ( t == ServiceVideoCategoryType.TwitchRecordings ) {
				Highlights = false;
			} else if ( t == ServiceVideoCategoryType.TwitchHighlights ) {
				Highlights = true;
			} else {
				throw new Exception( "Invalid type for TwitchUserInfo." );
			}

			_AutoDownload = node.Attributes["autoDownload"].Value == "true";
			UserID = node.Attributes["userID"].Value == "?" ? null : (long?)long.Parse( node.Attributes["userID"].Value );
			_LastRefreshedOn = Util.DateTimeFromUnixTime( ulong.Parse( node.Attributes["lastRefreshedOn"].Value ) );
			Username = node.Attributes["username"].Value;
			_Persistable = true;
		}

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", Type.ToString() );
			node.AppendAttribute( document, "autoDownload", AutoDownload ? "true" : "false" );
			node.AppendAttribute( document, "userID", UserID == null ? "?" : UserID.ToString() );
			node.AppendAttribute( document, "lastRefreshedOn", LastRefreshedOn.DateTimeToUnixTime().ToString() );
			node.AppendAttribute( document, "username", Username.ToString() );
			return node;
		}

		public override async Task<FetchReturnValue> Fetch( TwixelAPI.Twixel twitchApi, int offset, bool flat ) {
			List<IVideoInfo> videosToAdd = new List<IVideoInfo>();
			bool hasMore = true;
			long maxVideos = -1;
			int currentVideos = -1;

			if ( UserID == null ) {
				UserID = await twitchApi.GetUserId( Username, TwixelAPI.Twixel.APIVersion.v5 );
			}
			TwixelAPI.Total<List<TwixelAPI.Video>> broadcasts = await twitchApi.RetrieveVideos(
				channel: UserID == null ? Username : UserID.ToString(),
				offset: offset, limit: 25, broadcasts: !Highlights, hls: false,
				version: UserID == null ? TwixelAPI.Twixel.APIVersion.v3 : TwixelAPI.Twixel.APIVersion.v5
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

			if ( videosToAdd.Count <= 0 ) {
				return new FetchReturnValue { Success = true, HasMore = false, TotalVideos = maxVideos, VideoCountThisFetch = 0, Videos = videosToAdd };
			}

			return new FetchReturnValue { Success = true, HasMore = hasMore, TotalVideos = maxVideos, VideoCountThisFetch = currentVideos, Videos = videosToAdd };
		}
	}
}
