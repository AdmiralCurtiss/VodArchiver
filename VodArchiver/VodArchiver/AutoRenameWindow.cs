using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using VodArchiver.VideoInfo;
using VodArchiver.VideoJobs;

namespace VodArchiver {
	public partial class AutoRenameWindow : Form {
		private List<IVideoJob> Jobs;

		public AutoRenameWindow() {
			this.Jobs = new List<IVideoJob>();
			InitializeComponent();
		}

		public AutoRenameWindow( List<IVideoJob> jobs ) {
			this.Jobs = jobs;
			InitializeComponent();
		}

		private void buttonRename_Click( object sender, EventArgs e ) {
			string path = textBoxPath.Text.Trim();
			if ( path != "" ) {
				RenameTwitch( path );
				RenameHitbox( path );
			}
		}

		private static string ToUpperFirstChar( string s ) {
			if ( s.Length > 0 ) {
				return s.Substring( 0, 1 ).ToUpperInvariant() + s.Substring( 1 ).ToLowerInvariant();
			} else {
				return s;
			}
		}

		private void RenameTwitch( string path ) {
			foreach ( string file in Directory.EnumerateFiles( path, "twitch_*.mp4", SearchOption.AllDirectories ) ) {
				try {
					string filepart = Path.GetFileNameWithoutExtension( file );
					long tmp;
					string id = filepart.Split( '_' ).Where( x => x.StartsWith( "v" ) && long.TryParse( x.Substring( 1 ), out tmp ) ).First();

					List<IVideoJob> jobs = new List<IVideoJob>();
					foreach ( IVideoJob job in Jobs ) {
						if ( job != null ) {
							if ( job.VideoInfo.Service == StreamService.Twitch && job.VideoInfo.VideoId == id && job.JobStatus == VideoJobStatus.Finished ) {
								try {
									string game = Util.MakeStringFileSystemSafeBaseName( job.VideoInfo.VideoGame ).Replace( "-", "_" );
									game = string.Join( "", game.Split( new char[] { '_' }, StringSplitOptions.RemoveEmptyEntries ).Select( x => ToUpperFirstChar( x ) ) );

									string d = Path.GetDirectoryName( file );
									string f = Path.GetFileNameWithoutExtension( file );
									string e = Path.GetExtension( file );

									string[] parts = f.Split( new char[] { '_' }, StringSplitOptions.None );
									List<string> newparts = new List<string>();
									foreach ( string p in parts ) {
										newparts.Add( p );
										if ( p == id ) {
											newparts.Add( game );
										}
									}
									string nf = string.Join( "_", newparts.ToArray() );
									string newpath = Path.Combine( d, nf + e );
									if ( !File.Exists( newpath ) ) {
										File.Move( file, newpath );
									}

									break;
								} catch ( Exception ) {
								}
							}
						}
					}

				} catch ( Exception ) {
				}
			}
		}

		private void RenameHitbox( string path ) {
			foreach ( string file in Directory.EnumerateFiles( path, "hitbox_*.mp4", SearchOption.AllDirectories ) ) {
				try {
					string filepart = Path.GetFileNameWithoutExtension( file );
					long tmp;
					string id = filepart.Split( '_' ).Where( x => long.TryParse( x, out tmp ) ).Last();

					List<IVideoJob> jobs = new List<IVideoJob>();
					foreach ( IVideoJob job in Jobs ) {
						if ( job != null ) {
							if ( job.VideoInfo.Service == StreamService.Hitbox && job.VideoInfo.VideoId == id && job.JobStatus == VideoJobStatus.Finished ) {
								try {
									string game = Util.MakeStringFileSystemSafeBaseName( job.VideoInfo.VideoGame ).Replace( "-", "_" );
									game = string.Join( "", game.Split( new char[] { '_' }, StringSplitOptions.RemoveEmptyEntries ).Select( x => ToUpperFirstChar( x ) ) );

									string d = Path.GetDirectoryName( file );
									string f = Path.GetFileNameWithoutExtension( file );
									string e = Path.GetExtension( file );

									string[] parts = f.Split( new char[] { '_' }, StringSplitOptions.None );
									List<string> newparts = new List<string>();
									foreach ( string p in parts ) {
										newparts.Add( p );
										if ( p == id ) {
											newparts.Add( game );
										}
									}
									string nf = string.Join( "_", newparts.ToArray() );
									string newpath = Path.Combine( d, nf + e );
									if ( !File.Exists( newpath ) ) {
										File.Move( file, newpath );
									}

									break;
								} catch ( Exception ) {
								}
							}
						}
					}

				} catch ( Exception ) {
				}
			}
		}
	}
}
