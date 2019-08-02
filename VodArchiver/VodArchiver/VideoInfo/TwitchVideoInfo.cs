using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

namespace VodArchiver.VideoInfo {
	public class TwitchVideoInfo : IVideoInfo {
		TwitchVideo _Video;
		StreamService _Service;

		public TwitchVideoInfo( TwitchVideo video, StreamService service = StreamService.Twitch ) {
			_Video = video;
			_Service = service;
		}

		public TwitchVideoInfo( XmlNode node ) {
			_Video = DeserializeTwitchVideo( node.SelectSingleNode( "VideoInfo" ) );
			_Service = (StreamService)Enum.Parse( typeof( StreamService ), node.Attributes["service"].Value );
		}

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", "TwitchVideoInfo" );
			node.AppendChild( SerializeTwitchVideo( _Video, document, document.CreateElement( "VideoInfo" ) ) );
			node.AppendAttribute( document, "service", _Service.ToString() );
			return node;
		}

		public static XmlNode SerializeTwitchVideo( TwitchVideo video, XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", "TwitchVideo" );
			node.AppendAttribute( document, "id", video.ID.ToString( Util.SerializationFormatProvider ) );
			node.AppendAttribute( document, "userid", video.UserID.ToString( Util.SerializationFormatProvider ) );
			if ( video.Username != null )
				node.AppendAttribute( document, "username", video.Username );
			if ( video.Title != null )
				node.AppendAttribute( document, "title", video.Title );
			if ( video.Game != null )
				node.AppendAttribute( document, "game", video.Game );
			if ( video.Description != null )
				node.AppendAttribute( document, "description", video.Description );
			if ( video.CreatedAt != null )
				node.AppendAttribute( document, "created_at", video.CreatedAt.ToBinary().ToString( Util.SerializationFormatProvider ) );
			if ( video.PublishedAt != null )
				node.AppendAttribute( document, "published_at", video.PublishedAt.ToBinary().ToString( Util.SerializationFormatProvider ) );
			node.AppendAttribute( document, "duration", video.Duration.ToString( Util.SerializationFormatProvider ) );
			node.AppendAttribute( document, "view_count", video.ViewCount.ToString( Util.SerializationFormatProvider ) );
			node.AppendAttribute( document, "type", video.Type.ToString() );
			node.AppendAttribute( document, "state", video.State.ToString() );
			return node;
		}

		public static TwitchVideo DeserializeTwitchVideo( XmlNode node ) {
			TwitchVideo v = new TwitchVideo();
			if ( node.Attributes["_type"]?.Value == "TwixelAPI.Video" ) {
				// old twixel format, try to convert
				v.ID = long.Parse( node.Attributes["id"].Value.TrimStart( new char[] { 'a', 'v' } ), Util.SerializationFormatProvider );
				v.UserID = 0; // not stored in twixel format
				try {
					v.Username = node.SelectSingleNode( "channel" )?.DeserializeDictionary( ( string key ) => key, ( string value ) => value ).Where( x => x.Key.Contains( "display_name" ) ).First().Value;
				} catch ( Exception ) {
					v.Username = null;
				}
				v.Title = node.Attributes["title"]?.Value;
				v.Game = node.Attributes["game"]?.Value;
				v.Description = node.Attributes["description"]?.Value;
				if ( node.Attributes["recordedAt"]?.Value != null ) {
					v.CreatedAt = DateTime.FromBinary( long.Parse( node.Attributes["recordedAt"].Value, Util.SerializationFormatProvider ) );
					v.PublishedAt = v.CreatedAt;
				}
				v.Duration = long.Parse( node.Attributes["length"].Value, Util.SerializationFormatProvider );
				v.ViewCount = long.Parse( node.Attributes["views"].Value, Util.SerializationFormatProvider );
				v.Type = node.Attributes["broadcastType"]?.Value == "highlight" ? TwitchVideoType.Highlight : TwitchVideoType.Archive;
				v.State = node.Attributes["status"]?.Value == "recording" ? RecordingState.Live : RecordingState.Recorded;
			} else {
				v.ID = long.Parse( node.Attributes["id"].Value, Util.SerializationFormatProvider );
				v.UserID = long.Parse( node.Attributes["userid"].Value, Util.SerializationFormatProvider );
				v.Username = node.Attributes["username"]?.Value;
				v.Title = node.Attributes["title"]?.Value;
				v.Game = node.Attributes["game"]?.Value;
				v.Description = node.Attributes["description"]?.Value;
				if ( node.Attributes["created_at"]?.Value != null ) {
					v.CreatedAt = DateTime.FromBinary( long.Parse( node.Attributes["created_at"].Value, Util.SerializationFormatProvider ) );
				}
				if ( node.Attributes["published_at"]?.Value != null ) {
					v.PublishedAt = DateTime.FromBinary( long.Parse( node.Attributes["published_at"].Value, Util.SerializationFormatProvider ) );
				}
				v.Duration = long.Parse( node.Attributes["duration"].Value, Util.SerializationFormatProvider );
				v.ViewCount = long.Parse( node.Attributes["view_count"].Value, Util.SerializationFormatProvider );
				v.Type = (TwitchVideoType)Enum.Parse( typeof( TwitchVideoType ), node.Attributes["type"].Value );
				v.State = (RecordingState)Enum.Parse( typeof( RecordingState ), node.Attributes["state"].Value );
			}
			return v;
		}

		public override StreamService Service {
			get => _Service;
			set => _Service = value;
		}

		public override string Username {
			get => _Video.Username;
			set => _Video.Username = value;
		}

		public override string VideoGame {
			get => _Video.Game;
			set => _Video.Game = value;
		}

		public override string VideoId {
			get => _Video.ID.ToString( Util.SerializationFormatProvider );
			set => _Video.ID = long.Parse( value, Util.SerializationFormatProvider );
		}

		public override TimeSpan VideoLength {
			get => TimeSpan.FromSeconds( _Video.Duration );
			set => _Video.Duration = (long)value.TotalSeconds;
		}

		public override RecordingState VideoRecordingState {
			get => _Video.State;
			set => _Video.State = value;
		}

		public override DateTime VideoTimestamp {
			get => _Video.PublishedAt;
			set => _Video.PublishedAt = value;
		}

		public override string VideoTitle {
			get => _Video.Title;
			set => _Video.Title = value;
		}

		public override VideoFileType VideoType {
			get => VideoFileType.M3U;
			set => throw new InvalidOperationException();
		}
	}
}
