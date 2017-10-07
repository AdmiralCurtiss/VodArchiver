using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

namespace VodArchiver.VideoInfo {
	[Serializable]
	public class TwitchVideoInfo : IVideoInfo {
		TwixelAPI.Video VideoInfo;

		[System.Runtime.Serialization.OptionalField( VersionAdded = 2 )]
		StreamService _Service;

		public TwitchVideoInfo( TwixelAPI.Video video, StreamService service = StreamService.Twitch ) {
			VideoInfo = video;
			_Service = service;
		}

		public TwitchVideoInfo( XmlNode node ) {
			VideoInfo = DeserializeTwixelVideo( node.SelectSingleNode( "VideoInfo" ) );
			_Service = (StreamService)Enum.Parse( typeof( StreamService ), node.Attributes["service"].Value );
		}

		public override XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", "TwitchVideoInfo" );
			node.AppendChild( SerializeTwixelVideo( VideoInfo, document, document.CreateElement( "VideoInfo" ) ) );
			node.AppendAttribute( document, "service", _Service.ToString() );
			return node;
		}

		public static XmlNode SerializeTwixelVideo( TwixelAPI.Video video, XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "_type", "TwixelAPI.Video" );
			node.AppendAttribute( document, "_version", video.version.ToString() );
			if ( video.title != null ) node.AppendAttribute( document, "title", video.title );
			if ( video.description != null ) node.AppendAttribute( document, "description", video.description );
			node.AppendAttribute( document, "broadcastId", video.broadcastId.ToString() );
			if ( video.status != null ) node.AppendAttribute( document, "status", video.status );
			if ( video.tagList != null ) node.AppendAttribute( document, "tagList", video.tagList );
			if ( video.id != null ) node.AppendAttribute( document, "id", video.id );
			if ( video.recordedAt != null ) node.AppendAttribute( document, "recordedAt", video.recordedAt.ToBinary().ToString() );
			if ( video.game != null ) node.AppendAttribute( document, "game", video.game );
			node.AppendAttribute( document, "length", video.length.ToString() );
			if ( video.preview != null ) node.AppendAttribute( document, "preview", video.preview.OriginalString );
			if ( video.url != null ) node.AppendAttribute( document, "url", video.url.OriginalString );
			if ( video.fps != null ) node.AppendDictionary( document, "fps", video.fps, ( string s ) => s, ( double d ) => d.ToString() );
			node.AppendAttribute( document, "views", video.views.ToString() );
			if ( video.resolutions != null ) node.AppendDictionary( document, "resolutions", video.resolutions, ( string s ) => s, ( TwixelAPI.Resolution r ) => r.width + "x" + r.height );
			if ( video.broadcastType != null ) node.AppendAttribute( document, "broadcastType", video.broadcastType );
			if ( video.channel != null ) node.AppendDictionary( document, "channel", video.channel );
			if ( video.baseLinks != null ) node.AppendDictionary( document, "baseLinks", video.baseLinks, ( string s ) => s, ( Uri u ) => u.OriginalString );
			if ( video.previewv5 != null ) node.AppendDictionary( document, "previewv5", video.previewv5, ( string s ) => s, ( Uri u ) => u.OriginalString );
			if ( video.embed != null ) node.AppendAttribute( document, "embed", video.embed );
			if ( video.language != null ) node.AppendAttribute( document, "language", video.language );
			if ( video.viewable != null ) node.AppendAttribute( document, "viewable", video.viewable );
			if ( video.thumbnails != null ) {
				XmlElement element = document.CreateElement( "thumbnails" );
				foreach ( var kvp in video.thumbnails ) {
					XmlElement e = document.CreateElement( "_" + kvp.Key );
					e.AppendAttribute( document, "url", kvp.Value.url.OriginalString );
					e.AppendAttribute( document, "type", kvp.Value.type );
					element.AppendChild( e );
				}
				node.AppendChild( element );
			}

			return node;
		}

		public static TwixelAPI.Video DeserializeTwixelVideo( XmlNode node ) {
			TwixelAPI.Video v = new TwixelAPI.Video(
				node.Attributes["title"]?.Value,
				node.Attributes["description"]?.Value,
				long.Parse( node.Attributes["broadcastId"]?.Value ),
				node.Attributes["status"]?.Value,
				node.Attributes["tagList"]?.Value,
				node.Attributes["id"]?.Value,
				"2000-01-01", // temporary, this ctor is dumb
				node.Attributes["game"]?.Value,
				long.Parse( node.Attributes["length"]?.Value ),
				node.Attributes["preview"]?.Value,
				node.Attributes["url"]?.Value,
				long.Parse( node.Attributes["views"]?.Value ),
				null,
				null,
				node.Attributes["broadcastType"]?.Value,
				new Newtonsoft.Json.Linq.JObject(),
				null
			);
			v.version = (TwixelAPI.Twixel.APIVersion)Enum.Parse( typeof( TwixelAPI.Twixel.APIVersion ), node.Attributes["_version"]?.Value );
			v.recordedAt = DateTime.FromBinary( long.Parse( node.Attributes["recordedAt"]?.Value ) );
			v.fps = node.SelectSingleNode("fps")?.DeserializeDictionary( ( string key ) => key, ( string value ) => double.Parse( value ) );
			v.resolutions = node.SelectSingleNode( "resolutions" )?.DeserializeDictionary( ( string key ) => key, ( string value ) => new TwixelAPI.Resolution( int.Parse( value.Split('x')[0] ), int.Parse( value.Split( 'x' )[1] ) ) );
			v.channel = node.SelectSingleNode( "channel" )?.DeserializeDictionary( ( string key ) => key, ( string value ) => value );
			v.baseLinks = node.SelectSingleNode( "baseLinks" )?.DeserializeDictionary( ( string key ) => key, ( string value ) => new Uri( value ) );
			v.previewv5 = node.SelectSingleNode( "previewv5" )?.DeserializeDictionary( ( string key ) => key, ( string value ) => new Uri( value ) );
			v.embed = node.Attributes["embed"]?.Value;
			v.language = node.Attributes["language"]?.Value;
			v.viewable = node.Attributes["viewable"]?.Value;

			XmlNode thumbnailsNode = node.SelectSingleNode( "thumbnails" );
			if ( thumbnailsNode != null ) {
				v.thumbnails = new Dictionary<string, TwixelAPI.Thumbnail>();
				foreach ( XmlNode tn in thumbnailsNode.ChildNodes ) {
					string key = tn.Name.Substring( 1 );
					string url = tn.Attributes["url"].Value;
					string type = tn.Attributes["type"].Value;
					v.thumbnails.Add( key, new TwixelAPI.Thumbnail( url, type ) );
				}
			}

			return v;
		}

		public override StreamService Service {
			get {
				return _Service;
			}

			set {
				_Service = value;
			}
		}

		public override string Username {
			get {
				return VideoInfo.channel["name"];
			}

			set {
				VideoInfo.channel["name"] = value;
			}
		}

		public override string VideoGame {
			get {
				return VideoInfo.game;
			}

			set {
				VideoInfo.game = value;
			}
		}

		public override string VideoId {
			get {
				return VideoInfo.id;
			}

			set {
				VideoInfo.id = value;
			}
		}

		public override TimeSpan VideoLength {
			get {
				return TimeSpan.FromSeconds( VideoInfo.length );
			}

			set {
				VideoInfo.length = (long)value.TotalSeconds;
			}
		}

		public override RecordingState VideoRecordingState {
			get {
				switch ( VideoInfo.status ) {
					case "recording": return RecordingState.Live;
					case "recorded": return RecordingState.Recorded;
					default: return RecordingState.Unknown;
				}
			}

			set {
				switch ( value ) {
					case RecordingState.Live: VideoInfo.status = "recording"; break;
					case RecordingState.Recorded: VideoInfo.status = "recorded"; break;
					default: VideoInfo.status = "?"; break;
				}
			}
		}

		public override DateTime VideoTimestamp {
			get {
				return VideoInfo.recordedAt;
			}

			set {
				VideoInfo.recordedAt = value;
			}
		}

		public override string VideoTitle {
			get {
				return VideoInfo.title;
			}

			set {
				VideoInfo.title = value;
			}
		}

		public override VideoFileType VideoType {
			get {
				if ( VideoId.StartsWith( "v" ) ) { return VideoFileType.M3U; }
				if ( VideoId.StartsWith( "a" ) ) { return VideoFileType.FLVs; }
				return VideoFileType.Unknown;
			}

			set {
				throw new InvalidOperationException();
			}
		}

		[System.Runtime.Serialization.OnDeserializing]
		private void SetCountryRegionDefault( System.Runtime.Serialization.StreamingContext sc ) {
			_Service = StreamService.Twitch;
		}
	}
}
