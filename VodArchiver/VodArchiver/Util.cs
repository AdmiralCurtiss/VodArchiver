﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver {
	public class Util {
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
	}
}
