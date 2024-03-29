﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace VodArchiver {
	public static class Util {
		public static readonly IFormatProvider SerializationFormatProvider = System.Globalization.CultureInfo.InvariantCulture;

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
		public static string VodXmlPath { get { return System.IO.Path.Combine( PersistentDataPath, "downloads.bin" ); } }
		public static string VodXmlTempPath { get { return System.IO.Path.Combine( PersistentDataPath, "downloads.tmp" ); } }
		public static string UserSerializationPath { get { return System.IO.Path.Combine( PersistentDataPath, "users.txt" ); } }
		public static string UserSerializationXmlPath { get { return System.IO.Path.Combine( PersistentDataPath, "users.xml" ); } }
		public static string UserSerializationXmlTempPath { get { return System.IO.Path.Combine( PersistentDataPath, "users.tmp" ); } }

		public static string TwitchClientId { get { return Properties.Settings.Default.TwitchClientID; } }
		public static string TwitchClientSecret { get { return Properties.Settings.Default.TwitchClientSecret; } }
		public static string TwitchRedirectURI { get { return Properties.Settings.Default.TwitchRedirectURI; } }

		public static string YoutubeSpeedLimit { get { return Properties.Settings.Default.YoutubeSpeedLimit ?? ""; } }

		public static long MinimumFreeSpaceBytes { get { return Properties.Settings.Default.MinimumFreeSpaceBytes; } }
		public static long AbsoluteMinimumFreeSpaceBytes { get { return 50 * 1024 * 1024; } }

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

		public static DateTime DateTimeFromUnixTime( this ulong time ) {
			return new DateTime( 1970, 1, 1, 0, 0, 0, 0, DateTimeKind.Utc ).AddSeconds( time );
		}

		public static uint ParseDecOrHex( string s ) {
			s = s.Trim();

			if ( s.StartsWith( "0x" ) ) {
				s = s.Substring( 2 );
				return UInt32.Parse( s, System.Globalization.NumberStyles.HexNumber );
			} else {
				return UInt32.Parse( s );
			}
		}

		public static long ParseTwitchDuration(string v) {
			long duration = 0;
			StringBuilder sb = new StringBuilder();
			foreach (char c in v) {
				if (c >= '0' && c <= '9') {
					sb.Append(c);
				} else {
					long d = long.Parse(sb.ToString());
					switch (c) {
						case 'd':
							duration += d * 60 * 60 * 24;
							break;
						case 'h':
							duration += d * 60 * 60;
							break;
						case 'm':
							duration += d * 60;
							break;
						case 's':
							duration += d;
							break;
						default:
							throw new Exception("Invalid format: " + v);
					}
					sb.Clear();
				}
			}

			return duration;
		}

		public static string GetParameterFromUri( Uri uri, string parameter ) {
			foreach ( string q in uri.Query.Substring( 1 ).Split( '&' ) ) {
				var kvp = q.Split( new char[] { '=' }, 2 );
				if ( kvp.Length == 2 ) {
					string param = kvp[0];
					string value = kvp[1];
					if ( param == parameter ) {
						return value;
					}
				}
			}

			throw new Exception( "Uri '" + uri.ToString() + "' does not contain parameter '" + parameter + "'!" );
		}

		public static ushort ReadUInt16( this System.IO.Stream s ) {
			int b1 = s.ReadByte();
			int b2 = s.ReadByte();

			return (ushort)( b2 << 8 | b1 );
		}
		public static ushort PeekUInt16( this System.IO.Stream s ) {
			long pos = s.Position;
			ushort retval = s.ReadUInt16();
			s.Position = pos;
			return retval;
		}

		public static List<char> GetCustomInvalidFileNameChars() {
			List<char> chars = System.IO.Path.GetInvalidFileNameChars().ToList();
			chars.Add( ' ' ); // although spaces are technically valid we don't want them
			chars.Add( '.' ); // no dots either, just in case
			return chars;
		}

		public static string MakeStringFileSystemSafeBaseName( string s ) {
			string fn = s;
			foreach ( char c in GetCustomInvalidFileNameChars() ) {
				fn = fn.Replace( c, '_' );
			}
			return fn;
		}

		public static string FileSystemEscapeName(string s) {
			string fn = s;
			fn = fn.Replace("-", "--" + GetBytesAsHexString(Encoding.UTF8.GetBytes("-")) + "--");
			foreach (char c in GetCustomInvalidFileNameChars()) {
				fn = fn.Replace(c.ToString(), "--" + GetBytesAsHexString(Encoding.UTF8.GetBytes(c.ToString())) + "--");
			}
			return fn;
		}

		public static string GetBytesAsHexString(byte[] bytes) {
			StringBuilder sb = new StringBuilder();
			foreach (byte b in bytes) {
				sb.AppendFormat("{0:x2}", b);
			}
			return sb.ToString();
		}

		private static string ToUpperFirstChar( string s ) {
			if ( s.Length > 0 ) {
				bool maybeRomanNumeral = s.All( c => "IVX".Contains( c ) );
				if ( maybeRomanNumeral ) {
					return s;
				} else {
					return s.Substring( 0, 1 ).ToUpperInvariant() + s.Substring( 1 ).ToLowerInvariant();
				}
			} else {
				return s;
			}
		}
		public static string MakeIntercapsFilename( string gamename ) {
			string safename = Util.MakeStringFileSystemSafeBaseName( gamename ).Replace( "-", "_" );
			return string.Join( "", safename.Split( new char[] { '_' }, StringSplitOptions.RemoveEmptyEntries ).Select( x => ToUpperFirstChar( x ) ) );
		}
		public static string Crop( this string s, int length ) {
			return s.Length <= length ? s : s.Substring( 0, length );
		}

		public static void CopyStream( System.IO.Stream input, System.IO.Stream output, long count ) {
			byte[] buffer = new byte[4096];
			int read;

			long bytesLeft = count;
			while ( ( read = input.Read( buffer, 0, (int)Math.Min( buffer.LongLength, bytesLeft ) ) ) > 0 ) {
				output.Write( buffer, 0, read );
				bytesLeft -= read;
				if ( bytesLeft <= 0 )
					return;
			}
		}

		public static SemaphoreSlim ExpensiveDiskIOSemaphore = new SemaphoreSlim( 1 );

		public static object JobFileLock = new object();
	}
}
