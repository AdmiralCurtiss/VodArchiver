using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver {
	public class VideoDeadException : Exception {
		public VideoDeadException() {
		}

		public VideoDeadException( string message ) : base( message ) {
		}

		public VideoDeadException( string message, Exception innerException ) : base( message, innerException ) {
		}

		protected VideoDeadException( SerializationInfo info, StreamingContext context ) : base( info, context ) {
		}
	}
}
