using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver {
	class RetryLaterException : Exception {
		public RetryLaterException() {
		}

		public RetryLaterException( string message ) : base( message ) {
		}

		public RetryLaterException( string message, Exception innerException ) : base( message, innerException ) {
		}

		protected RetryLaterException( SerializationInfo info, StreamingContext context ) : base( info, context ) {
		}
	}
}
