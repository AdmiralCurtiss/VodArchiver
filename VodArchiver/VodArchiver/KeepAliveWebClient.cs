using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver {
	class KeepAliveWebClient : WebClient {
		protected override WebRequest GetWebRequest( Uri address ) {
			var webRequest = base.GetWebRequest( address );
			var httpRequest = webRequest as HttpWebRequest;
			if ( httpRequest != null ) {
				httpRequest.ServicePoint.SetTcpKeepAlive( true, 10000, 5000 );
			}
			return webRequest;
		}
	}
}
