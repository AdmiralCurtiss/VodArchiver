using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver {
	public class ExternalProgramReturnNonzeroException : Exception {
		public int ReturnValue;
		public string Program;
		public string[] Arguments;
		public string StdOut;
		public string StdErr;
		public override string ToString() {
			return Program + " returned " + ReturnValue + ": " + ( StdErr.Trim() != "" ? StdErr : StdOut );
		}
	}

	public class ExternalProgramExecution {
		public static string EscapeArgument( string arg ) {
			// TODO: this isn't enough
			if ( arg.IndexOfAny( new char[] { '"', ' ', '\t', '\\' } ) == -1 ) {
				return "\"" + arg.Replace( "\\", "\\\\" ).Replace( "\"", "\\\"" ) + "\"";
			} else {
				return arg;
			}
		}

		public struct RunProgramReturnValue { public string StdOut; public string StdErr; }
		public static async Task<RunProgramReturnValue> RunProgram(
			string prog,
			string[] args,
			string workingDir = null,
			System.Diagnostics.DataReceivedEventHandler[] stdoutCallbacks = null,
			System.Diagnostics.DataReceivedEventHandler[] stderrCallbacks = null,
			bool youtubeSpeedWorkaround = false
		) {
			return await Task.Run(() => RunProgramSynchronous(prog, args, workingDir, stdoutCallbacks, stderrCallbacks, youtubeSpeedWorkaround));
		}

		public static RunProgramReturnValue RunProgramSynchronous(
			string prog,
			string[] args,
			string workingDir = null,
			System.Diagnostics.DataReceivedEventHandler[] stdoutCallbacks = null,
			System.Diagnostics.DataReceivedEventHandler[] stderrCallbacks = null,
			bool youtubeSpeedWorkaround = false
		) {
			System.Diagnostics.ProcessStartInfo startInfo = new System.Diagnostics.ProcessStartInfo();
			startInfo.WorkingDirectory = workingDir == null ? "" : workingDir;
			startInfo.CreateNoWindow = true;
			startInfo.UseShellExecute = false;
			startInfo.FileName = prog;
			startInfo.WindowStyle = System.Diagnostics.ProcessWindowStyle.Hidden;

			StringBuilder sb = new StringBuilder();
			foreach ( string s in args ) {
				sb.Append( EscapeArgument( s ) ).Append( " " );
			}
			startInfo.Arguments = sb.ToString();

			startInfo.RedirectStandardOutput = true;
			startInfo.RedirectStandardError = true;
			startInfo.RedirectStandardInput = false;

			using ( System.Diagnostics.Process exeProcess = new System.Diagnostics.Process() ) {
				int youtubeSpeedWorkaroundCounter = 0;

				StringBuilder outputData = new StringBuilder();
				StringBuilder errorData = new StringBuilder();
				exeProcess.OutputDataReceived += ( sender, received ) => {
					if ( !String.IsNullOrEmpty( received.Data ) ) {
						outputData.Append( received.Data );

						if (youtubeSpeedWorkaround) {
							if (IsSlowYoutubeSpeed(received.Data)) {
								++youtubeSpeedWorkaroundCounter;
								if (youtubeSpeedWorkaroundCounter > 100) {
									try {
										exeProcess.Kill();
									} catch (Exception) { }
								}
							} else {
								youtubeSpeedWorkaroundCounter = 0;
							}
						}
					}
				};
				if ( stdoutCallbacks != null ) {
					foreach ( System.Diagnostics.DataReceivedEventHandler d in stdoutCallbacks ) {
						exeProcess.OutputDataReceived += d;
					}
				}

				exeProcess.ErrorDataReceived += ( sender, received ) => {
					if ( !String.IsNullOrEmpty( received.Data ) ) {
						errorData.Append( received.Data );
					}
				};
				if ( stderrCallbacks != null ) {
					foreach ( System.Diagnostics.DataReceivedEventHandler d in stderrCallbacks ) {
						exeProcess.ErrorDataReceived += d;
					}
				}

				exeProcess.StartInfo = startInfo;
				exeProcess.Start();
				exeProcess.PriorityClass = System.Diagnostics.ProcessPriorityClass.Idle;
				exeProcess.BeginOutputReadLine();
				exeProcess.BeginErrorReadLine();
				exeProcess.WaitForExit();

				string output = outputData.ToString();
				string err = errorData.ToString();

				if ( exeProcess.ExitCode != 0 ) {
					throw new ExternalProgramReturnNonzeroException() { ReturnValue = exeProcess.ExitCode, Program = prog, Arguments = args, StdOut = output, StdErr = err };
				}

				return new RunProgramReturnValue() { StdOut = output, StdErr = err };
			}
		}

		private static bool IsSlowYoutubeSpeed(string data) {
			try {
				if (data == null || !data.StartsWith("[download]"))
					return false;
				int kibsidx = data.IndexOf("KiB/s");
				if (kibsidx == -1)
					return false;
				int previdx = kibsidx - 1;
				while (true) {
					if (previdx < 0)
						return false;
					if (data[previdx] == ' ')
						break;
					--previdx;
				}
				string kibss = data.Substring(previdx + 1, kibsidx - (previdx + 1));
				float kibs;
				if (!float.TryParse(kibss, System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture, out kibs))
					return false;
				if (kibs >= 100.0f)
					return false;

				int etaidx = data.LastIndexOf("ETA ");
				if (etaidx == -1)
					return true;
				etaidx += 4;
				string etastr = data.Substring(etaidx, data.Length - etaidx);
				int coloncount = etastr.Count(x => x == ':');
				if (coloncount != 1)
					return coloncount > 1;
				var minsec = etastr.Split(':');
				return int.Parse(minsec[0]) > 59;
			} catch (Exception) {
				return false;
			}
		}
	}
}
