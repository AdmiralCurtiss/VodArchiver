using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace VodArchiver {
	static class Program {
		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main() {
			System.Net.HttpWebRequest.DefaultMaximumErrorResponseLength = -1;
			System.Net.HttpWebRequest.DefaultMaximumResponseHeadersLength = -1;

			Application.EnableVisualStyles();
			Application.SetCompatibleTextRenderingDefault( false );
			Application.Run( new DownloadForm() );
		}
	}
}
