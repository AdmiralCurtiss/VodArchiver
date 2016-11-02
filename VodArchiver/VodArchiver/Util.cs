using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace VodArchiver {
	public static class Util {
		public static string TargetFolderPath {
			get {
				if ( String.IsNullOrWhiteSpace( Properties.Settings.Default.TargetFolder ) ) {
					return System.IO.Path.Combine( Environment.GetFolderPath( Environment.SpecialFolder.MyVideos ), "VodArchiver" );
				} else {
					return Properties.Settings.Default.TargetFolder;
				}
			}
		}

		public static string TempFolderPath {
			get {
				if ( String.IsNullOrWhiteSpace( Properties.Settings.Default.TempFolder ) ) {
					return System.IO.Path.Combine( TargetFolderPath, "Temp" );
				} else {
					return Properties.Settings.Default.TempFolder;
				}
			}
		}

		public static bool EnableCustomDataPath { get { return Properties.Settings.Default.EnableCustomDataPath; } }
		public static string PersistentDataPath {
			get {
				if ( EnableCustomDataPath && !String.IsNullOrWhiteSpace( Properties.Settings.Default.CustomDataPath ) ) {
					return Properties.Settings.Default.CustomDataPath;
				} else {
					return System.Windows.Forms.Application.LocalUserAppDataPath;
				}
			}
		}

		public static string VodBinaryPath { get { return System.IO.Path.Combine( PersistentDataPath, "vods.bin" ); } }
		public static string VodBinaryTempPath { get { return System.IO.Path.Combine( PersistentDataPath, "vods.tmp" ); } }
		public static string UserSerializationPath { get { return System.IO.Path.Combine( PersistentDataPath, "users.txt" ); } }

		public static string TwitchClientId { get { return Properties.Settings.Default.TwitchClientID; } }
		public static string TwitchRedirectURI { get { return Properties.Settings.Default.TwitchRedirectURI; } }

		public static bool ShowDownloadFetched {
			get {
				return Properties.Settings.Default.ShowDownloadFetched;
			}
		}

		public static bool ShowAnySpecialButton {
			get {
				return ShowDownloadFetched;
			}
		}

		public static bool AllowTimedAutoFetch {
			get {
				return ShowDownloadFetched;
			}
		}

		public static string AppUserModelId {
			get {
				return "AdmiralCurtiss.VodArchiver";
			}
		}
		public static bool ShowToastNotifications {
			get {
				return true;
			}
		}

		public static async Task<bool> FileExists( string file ) {
			return await Task.Run( () => System.IO.File.Exists( file ) );
		}

		public static async Task DeleteFile( string file ) {
			await Task.Run( () => System.IO.File.Delete( file ) );
		}

		public static async Task DeleteFiles( string[] files ) {
			foreach ( string file in files ) {
				await DeleteFile( file );
			}
		}

		public static void MoveFileOverwrite( string sourceName, string targetName ) {
			if ( System.IO.File.Exists( targetName ) ) {
				System.IO.File.Delete( targetName );
			}
			System.IO.File.Move( sourceName, targetName );
		}

		public static bool AddOrUpdate<T>( this SortedSet<T> set, T item ) {
			if ( set.Contains( item ) ) {
				set.Remove( item );
			}
			return set.Add( item );
		}

		public static ulong DateTimeToUnixTime( this DateTime time ) {
			return (ulong)( time - new DateTime( 1970, 1, 1, 0, 0, 0, 0, DateTimeKind.Utc ) ).TotalSeconds;
		}

		public static SemaphoreSlim ExpensiveDiskIOSemaphore = new SemaphoreSlim( 1 );

		public static object JobFileLock = new object();
	}
}
