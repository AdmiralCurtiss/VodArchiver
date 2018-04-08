using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver {
	public static class FFMpegUtil {
		public static async Task<FFProbeResult> Probe( string filename ) {
			ExternalProgramExecution.RunProgramReturnValue retval = await VodArchiver.ExternalProgramExecution.RunProgram(
				"ffprobe",
				new string[] {
					"-show_format",
					"-show_streams",
					"-print_format", "json",
					filename
				}
			);

			JObject jo = JObject.Parse( retval.StdOut );
			var jsonFormat = jo["format"];

			string path = System.IO.Path.GetFullPath( filename );
			DateTime timestamp = System.IO.File.GetCreationTimeUtc( filename );
			ulong filesize = (ulong)jsonFormat["size"];
			ulong bitrate = (ulong)jsonFormat["bit_rate"];
			TimeSpan duration = TimeSpan.FromSeconds( (double)jsonFormat["duration"] );
			List<FFProbeStream> streams = new List<FFProbeStream>();

			foreach ( JToken jsonStream in jo["streams"] ) {
				streams.Add( new FFProbeStream() {
					Index = (uint)jsonStream["index"],
					CodecTag = Util.ParseDecOrHex( jsonStream["codec_tag"].ToString() ),
					CodecType = jsonStream["codec_type"].ToString(),
					Duration = TimeSpan.FromSeconds( (double)jsonStream["duration"] ),
				} );
			}

			return new FFProbeResult() {
				Path = path,
				Timestamp = timestamp,
				Filesize = filesize,
				Bitrate = bitrate,
				Duration = duration,
				Streams = streams,
			};
		}
	}

	public class FFProbeResult {
		public string Path;
		public DateTime Timestamp;
		public ulong Filesize;
		public ulong Bitrate;
		public TimeSpan Duration;
		public List<FFProbeStream> Streams;
	}

	public class FFProbeStream {
		public uint Index;
		public uint CodecTag;
		public string CodecType;
		public TimeSpan Duration;
	}
}
