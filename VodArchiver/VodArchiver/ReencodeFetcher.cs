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
		public async static Task<List<IVideoInfo>> FetchReencodeableFiles( string path, string additionalOptions ) {
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
				FFProbeResult probe = await FFMpegUtil.Probe( f );

				// super hacky... probably should improve this stuff
				List<string> ffmpegOptions;
				string postfixOld;
				string postfixNew;
				if ( additionalOptions.Contains( "rescale720" ) ) {
					ffmpegOptions = new List<string>() { "-c:v", "libx264", "-preset", "slower", "-crf", "23", "-sws_flags", "lanczos", "-vf", "\"scale=-2:720\"", "-c:a", "copy", "-max_muxing_queue_size", "100000" };
					postfixOld = "_chunked";
					postfixNew = "_x264crf23scaled720p";
				} else if ( additionalOptions.Contains( "rescale480" ) ) {
					ffmpegOptions = new List<string>() { "-c:v", "libx264", "-preset", "slower", "-crf", "23", "-sws_flags", "lanczos", "-vf", "\"scale=-2:480\"", "-c:a", "copy", "-max_muxing_queue_size", "100000" };
					postfixOld = "_chunked";
					postfixNew = "_x264crf23scaled480p";
				} else {
					ffmpegOptions = new List<string>() { "-c:v", "libx264", "-preset", "slower", "-crf", "23", "-c:a", "copy", "-max_muxing_queue_size", "100000" };
					postfixOld = "_chunked";
					postfixNew = "_x264crf23";
				}

				IVideoInfo retval = new FFMpegReencodeJobVideoInfo( f, probe, ffmpegOptions, postfixOld, postfixNew );

				rv.Add( retval );
			}
			return rv;
		}
	}
}
