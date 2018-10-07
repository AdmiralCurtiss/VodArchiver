using System;
using System.Collections.Generic;
using System.IO;
using System.Windows.Forms;

namespace VodArchiver
{
	public partial class VideoSplitWindow : Form
	{
		public VideoSplitWindow() {
			InitializeComponent();
		}

		private void buttonBrowse_Click( object sender, EventArgs e ) {
			OpenFileDialog d = new OpenFileDialog();
			if ( d.ShowDialog() == DialogResult.OK ) {
				this.textBoxFilename.Text = d.FileName;
			}
		}

		private async void buttonSplit_Click( object sender, EventArgs e ) {
			if ( !this.buttonSplit.Enabled || this.textBoxFilename.Text == "" || this.textBoxSplits.Text == "" ) {
				return;
			}

			try {
				this.buttonSplit.Enabled = false;

				string outname = GenerateOutputName( this.textBoxFilename.Text );
				List<string> args = new List<string>();
				args.Add( "-i" );
				args.Add( this.textBoxFilename.Text );
				args.Add( "-codec" );
				args.Add( "copy" );
				args.Add( "-map" );
				args.Add( "0" );
				args.Add( "-f" );
				args.Add( "segment" );
				args.Add( "-segment_times" );
				args.Add( this.textBoxSplits.Text );
				args.Add( outname );
				args.Add( "-start_number" );
				args.Add( "1" );

				bool existedBefore = File.Exists( outname.Replace( "%d", "0" ) );

				try {
					await Util.ExpensiveDiskIOSemaphore.WaitAsync();
				} catch ( OperationCanceledException ) {
					return;
				}
				try {
					await VodArchiver.ExternalProgramExecution.RunProgram( "ffmpeg", args.ToArray() );
				} finally {
					Util.ExpensiveDiskIOSemaphore.Release();
				}


				if ( !existedBefore && File.Exists( outname.Replace( "%d", "0" ) ) ) {
					// output generated with indices starting at 0 instead of 1, rename
					List<(string input, string output)> l = new List<(string input, string output)>();
					int digit = 0;
					while ( true ) {
						string input = outname.Replace( "%d", digit.ToString() );
						string output = outname.Replace( "%d", ( digit + 1 ).ToString() );
						if ( File.Exists( input ) ) {
							l.Add( (input, output) );
						} else {
							break;
						}
						++digit;
					}

					l.Reverse();

					foreach ( var (input, output) in l ) {
						if ( !File.Exists( output ) ) {
							File.Move( input, output );
						}
					}
				}

				this.textBoxFilename.Text = "";
				this.textBoxSplits.Text = "";
			} finally {
				this.buttonSplit.Enabled = true;
			}
		}

		private string GenerateOutputName( string inputName ) {
			string dir = System.IO.Path.GetDirectoryName( inputName );
			string fn = System.IO.Path.GetFileNameWithoutExtension( inputName );
			string ext = System.IO.Path.GetExtension( inputName );
			var split = fn.Split( new char[] { '_' }, StringSplitOptions.None );
			List<string> output = new List<string>();

			bool added = false;
			foreach ( string s in split ) {
				ulong val;
				if ( !added && s.StartsWith( "v" ) && ulong.TryParse( s.Substring( 1 ), out val ) ) {
					output.Add( s + "-p%d" );
					added = true;
				} else {
					output.Add( s );
				}
			}

			if ( !added ) {
				output.Add( "%d" );
			}

			return System.IO.Path.Combine( dir, string.Join( "_", output ) + ext );
		}
	}
}
