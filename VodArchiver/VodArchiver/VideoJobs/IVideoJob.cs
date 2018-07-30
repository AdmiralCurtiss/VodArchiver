using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Xml;
using VodArchiver.VideoInfo;

namespace VodArchiver.VideoJobs {
	public enum VideoJobStatus {
		NotStarted,
		Running,
		Finished,
		Dead,
	}

	public enum ResultType {
		Failure,
		Success,
		Cancelled,
		Dead,
		TemporarilyUnavailable,
	}

	public abstract class IVideoJob {
		private string _Status;
		public virtual string Status {
			get {
				return _Status;
			}
			set {
				_Status = value;
				StatusUpdater.Update();
			}
		}

		public virtual string HumanReadableJobName {
			get {
				if ( VideoInfo != null ) {
					if ( !String.IsNullOrEmpty( VideoInfo.Username ) ) {
						return VideoInfo.Username + "/" + VideoInfo.VideoId + " (" + VideoInfo.Service + ")";
					} else {
						return VideoInfo.VideoId + " (" + VideoInfo.Service + ")";
					}
				} else {
					return "Unknown Video";
				}
			}
		}

		public StatusUpdate.IStatusUpdate StatusUpdater;

		public long Index;

		public VideoJobStatus JobStatus;

		public virtual VideoInfo.IVideoInfo VideoInfo { get; set; }

		public bool HasBeenValidated;

		public string Notes;

		public DateTime JobStartTimestamp;

		public DateTime JobFinishTimestamp;

		public IVideoJob() { }

		public IVideoJob( XmlNode node ) {
			_Status = node.Attributes["textStatus"].Value;
			JobStatus = (VideoJobStatus)Enum.Parse( typeof( VideoJobStatus ), node.Attributes["jobStatus"].Value );
			VideoInfo = IVideoInfo.Deserialize( node.SelectSingleNode( "VideoInfo" ) );
			HasBeenValidated = bool.Parse( node.Attributes["hasBeenValidated"].Value );
			Notes =  node.Attributes["notes"].Value;
			JobStartTimestamp = DateTime.FromBinary( long.Parse( node.Attributes["jobStartTimestamp"].Value, Util.SerializationFormatProvider ) );
			JobFinishTimestamp = DateTime.FromBinary( long.Parse( node.Attributes["jobFinishTimestamp"].Value, Util.SerializationFormatProvider ) );
		}

		public abstract Task<ResultType> Run( CancellationToken cancellationToken );

		public virtual XmlNode Serialize( XmlDocument document, XmlNode node ) {
			node.AppendAttribute( document, "textStatus", _Status );
			node.AppendAttribute( document, "jobStatus", JobStatus.ToString() );
			node.AppendChild( VideoInfo.Serialize( document, document.CreateElement( "VideoInfo" ) ) );
			node.AppendAttribute( document, "hasBeenValidated", HasBeenValidated.ToString( Util.SerializationFormatProvider ) );
			node.AppendAttribute( document, "notes", Notes );
			node.AppendAttribute( document, "jobStartTimestamp", JobStartTimestamp.ToBinary().ToString( Util.SerializationFormatProvider ) );
			node.AppendAttribute( document, "jobFinishTimestamp", JobFinishTimestamp.ToBinary().ToString( Util.SerializationFormatProvider ) );
			return node;
		}

		public static IVideoJob Deserialize( XmlNode node ) {
			string type = node.Attributes["_type"].Value;
			switch ( type ) {
				case "GenericFileJob": return new GenericFileJob( node );
				case "HitboxVideoJob": return new HitboxVideoJob( node );
				case "TwitchChatReplayJob": return new TwitchChatReplayJob( node );
				case "TwitchVideoJob": return new TwitchVideoJob( node );
				case "YoutubeVideoJob": return new YoutubeVideoJob( node );
				case "FFMpegReencodeJob": return new FFMpegReencodeJob( node );
				default: throw new Exception( "Unknown video job type: " + type );
			}
		}

		public override bool Equals( object obj ) {
			return Equals( obj as IVideoJob );
		}

		public bool Equals( IVideoJob other ) {
			return other != null && VideoInfo.Equals( other.VideoInfo );
		}

		public override int GetHashCode() {
			return VideoInfo.GetHashCode();
		}
	}
}
