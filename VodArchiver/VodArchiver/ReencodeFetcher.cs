using Argotic.Syndication;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using VodArchiver.VideoInfo;

namespace VodArchiver {
	class ReencodeFetcher {
		public async static Task<List<IVideoInfo>> FetchReencodeableFiles( string path ) {
			string chunked = "_chunked";
			string postfix = "_x264crf23";

			List<string> reencodeFiles = new List<string>();

			foreach ( string file in System.IO.Directory.EnumerateFiles( path ) ) {
				string name = System.IO.Path.GetFileNameWithoutExtension( file );
				string ext = Path.GetExtension( file );

				if ( ext == ".mp4" && name.EndsWith( chunked ) ) {
					string newfile = Path.Combine( path, postfix, name.Substring( 0, name.Length - chunked.Length ) + postfix + ext );
					if ( !File.Exists( newfile ) ) {
						reencodeFiles.Add( System.IO.Path.GetFullPath( file ) );
					}
				}
			}

			List<IVideoInfo> rv = new List<IVideoInfo>();
			foreach ( string f in reencodeFiles ) {
				IVideoInfo info = await VideoJobs.FFMpegReencodeJob.Probe( f );
				rv.Add( info );
			}
			return rv;
		}
	}
}
