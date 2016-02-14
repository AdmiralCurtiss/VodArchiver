﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Net.Http;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using TwixelAPI;
using VodArchiver.VideoJobs;

namespace VodArchiver {
	public partial class Form1 : Form {
		Twixel TwitchAPI;
		System.Collections.Concurrent.ConcurrentQueue<IVideoJob> JobQueue;
		private int RunningJobs;
		private object Lock = new object();

		public Form1() {
			InitializeComponent();
			comboBoxService.SelectedIndex = 0;
			TwitchAPI = new Twixel( "", "", Twixel.APIVersion.v3 );
			JobQueue = new System.Collections.Concurrent.ConcurrentQueue<IVideoJob>();
			RunningJobs = 0;
		}

		private async void buttonDownload_Click( object sender, EventArgs e ) {
			string id = textboxMediaId.Text.Trim();
			if ( id == "" ) { return; }

			IVideoJob job;
			switch ( comboBoxService.Text ) {
				case "Twitch":
					job = new TwitchVideoJob( TwitchAPI, id );
					break;
				case "Hitbox":
					job = new HitboxVideoJob( id );
					break;
				default:
					throw new Exception( comboBoxService.SelectedText + " is not a valid service." );
			}

			job.StatusUpdater = new StatusUpdate.ObjectListViewStatusUpdate( objectListViewDownloads, job );
			objectListViewDownloads.AddObject( job );
			job.Status = "Waiting...";
			JobQueue.Enqueue( job );

			await RunJob();
		}

		private async Task RunJob() {
			bool runNewJob = false;
			IVideoJob job = null;
			lock ( Lock ) {
				if ( RunningJobs < 3 && JobQueue.TryDequeue( out job ) ) {
					++RunningJobs;
					runNewJob = true;
				}
			}

			if ( runNewJob ) {
				try {
					await job.Run();
				} catch ( Exception ex ) {
					job.Status = "ERROR: " + ex.ToString();
				}

				lock ( Lock ) {
					--RunningJobs;
				}
				await RunJob();
			}
		}

		private void buttonSettings_Click( object sender, EventArgs e ) {
			new SettingsWindow().ShowDialog();
		}
	}
}