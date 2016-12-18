namespace VodArchiver {
	partial class DownloadForm {
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose( bool disposing ) {
			if ( disposing && ( components != null ) ) {
				components.Dispose();
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent() {
			this.buttonDownload = new System.Windows.Forms.Button();
			this.labelMediaId = new System.Windows.Forms.Label();
			this.textboxMediaId = new System.Windows.Forms.TextBox();
			this.comboBoxService = new System.Windows.Forms.ComboBox();
			this.objectListViewDownloads = new BrightIdeasSoftware.ObjectListView();
			this.columnIndex = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.columnValidated = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.columnService = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.columnVideoId = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.columnUsername = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.columnTitle = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.columnGame = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.columnTimestamp = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.columnLength = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.columnRecordingState = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.columnNotes = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.columnAction1 = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.columnActionRemove = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.columnStatus = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.buttonSettings = new System.Windows.Forms.Button();
			this.buttonFetchUser = new System.Windows.Forms.Button();
			this.labelStatusBar = new System.Windows.Forms.Label();
			this.labelPowerStateWhenDone = new System.Windows.Forms.Label();
			this.comboBoxPowerStateWhenDone = new System.Windows.Forms.ComboBox();
			((System.ComponentModel.ISupportInitialize)(this.objectListViewDownloads)).BeginInit();
			this.SuspendLayout();
			// 
			// buttonDownload
			// 
			this.buttonDownload.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonDownload.Location = new System.Drawing.Point(1291, 12);
			this.buttonDownload.Name = "buttonDownload";
			this.buttonDownload.Size = new System.Drawing.Size(75, 20);
			this.buttonDownload.TabIndex = 0;
			this.buttonDownload.Text = "Download";
			this.buttonDownload.UseVisualStyleBackColor = true;
			this.buttonDownload.Click += new System.EventHandler(this.buttonDownload_Click);
			// 
			// labelMediaId
			// 
			this.labelMediaId.AutoSize = true;
			this.labelMediaId.Location = new System.Drawing.Point(266, 15);
			this.labelMediaId.Name = "labelMediaId";
			this.labelMediaId.Size = new System.Drawing.Size(87, 13);
			this.labelMediaId.TabIndex = 1;
			this.labelMediaId.Text = "URL or Media ID";
			// 
			// textboxMediaId
			// 
			this.textboxMediaId.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.textboxMediaId.Location = new System.Drawing.Point(359, 12);
			this.textboxMediaId.Name = "textboxMediaId";
			this.textboxMediaId.Size = new System.Drawing.Size(926, 20);
			this.textboxMediaId.TabIndex = 2;
			// 
			// comboBoxService
			// 
			this.comboBoxService.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.comboBoxService.FormattingEnabled = true;
			this.comboBoxService.Items.AddRange(new object[] {
            "Autodetect",
            "Twitch",
            "Hitbox",
            "Youtube"});
			this.comboBoxService.Location = new System.Drawing.Point(165, 13);
			this.comboBoxService.Name = "comboBoxService";
			this.comboBoxService.Size = new System.Drawing.Size(89, 21);
			this.comboBoxService.TabIndex = 3;
			// 
			// objectListViewDownloads
			// 
			this.objectListViewDownloads.AllColumns.Add(this.columnIndex);
			this.objectListViewDownloads.AllColumns.Add(this.columnValidated);
			this.objectListViewDownloads.AllColumns.Add(this.columnService);
			this.objectListViewDownloads.AllColumns.Add(this.columnVideoId);
			this.objectListViewDownloads.AllColumns.Add(this.columnUsername);
			this.objectListViewDownloads.AllColumns.Add(this.columnTitle);
			this.objectListViewDownloads.AllColumns.Add(this.columnGame);
			this.objectListViewDownloads.AllColumns.Add(this.columnTimestamp);
			this.objectListViewDownloads.AllColumns.Add(this.columnLength);
			this.objectListViewDownloads.AllColumns.Add(this.columnRecordingState);
			this.objectListViewDownloads.AllColumns.Add(this.columnNotes);
			this.objectListViewDownloads.AllColumns.Add(this.columnAction1);
			this.objectListViewDownloads.AllColumns.Add(this.columnActionRemove);
			this.objectListViewDownloads.AllColumns.Add(this.columnStatus);
			this.objectListViewDownloads.AlternateRowBackColor = System.Drawing.Color.FromArgb(((int)(((byte)(240)))), ((int)(((byte)(240)))), ((int)(((byte)(240)))));
			this.objectListViewDownloads.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.objectListViewDownloads.CellEditUseWholeCell = false;
			this.objectListViewDownloads.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnIndex,
            this.columnValidated,
            this.columnService,
            this.columnVideoId,
            this.columnUsername,
            this.columnTitle,
            this.columnGame,
            this.columnTimestamp,
            this.columnLength,
            this.columnRecordingState,
            this.columnNotes,
            this.columnAction1,
            this.columnActionRemove,
            this.columnStatus});
			this.objectListViewDownloads.Cursor = System.Windows.Forms.Cursors.Default;
			this.objectListViewDownloads.HighlightBackgroundColor = System.Drawing.SystemColors.Highlight;
			this.objectListViewDownloads.HighlightForegroundColor = System.Drawing.Color.Empty;
			this.objectListViewDownloads.Location = new System.Drawing.Point(12, 41);
			this.objectListViewDownloads.Name = "objectListViewDownloads";
			this.objectListViewDownloads.SelectedBackColor = System.Drawing.SystemColors.Highlight;
			this.objectListViewDownloads.ShowCommandMenuOnRightClick = true;
			this.objectListViewDownloads.ShowItemCountOnGroups = true;
			this.objectListViewDownloads.Size = new System.Drawing.Size(1354, 404);
			this.objectListViewDownloads.TabIndex = 4;
			this.objectListViewDownloads.TintSortColumn = true;
			this.objectListViewDownloads.UseAlternatingBackColors = true;
			this.objectListViewDownloads.UseCompatibleStateImageBehavior = false;
			this.objectListViewDownloads.UseFilterIndicator = true;
			this.objectListViewDownloads.UseFiltering = true;
			this.objectListViewDownloads.UseHotControls = false;
			this.objectListViewDownloads.View = System.Windows.Forms.View.Details;
			this.objectListViewDownloads.ButtonClick += new System.EventHandler<BrightIdeasSoftware.CellClickEventArgs>(this.objectListViewDownloads_ButtonClick);
			this.objectListViewDownloads.CellEditStarting += new BrightIdeasSoftware.CellEditEventHandler(this.objectListViewDownloads_CellEditStarting);
			// 
			// columnIndex
			// 
			this.columnIndex.Groupable = false;
			this.columnIndex.IsEditable = false;
			this.columnIndex.Text = "Index";
			// 
			// columnValidated
			// 
			this.columnValidated.AspectName = "HasBeenValidated";
			this.columnValidated.CheckBoxes = true;
			this.columnValidated.Text = "Valid";
			// 
			// columnService
			// 
			this.columnService.AspectName = "VideoInfo.Service";
			this.columnService.IsEditable = false;
			this.columnService.Text = "Service";
			this.columnService.Width = 78;
			// 
			// columnVideoId
			// 
			this.columnVideoId.AspectName = "VideoInfo.VideoId";
			this.columnVideoId.IsEditable = false;
			this.columnVideoId.Text = "Video ID";
			this.columnVideoId.Width = 68;
			// 
			// columnUsername
			// 
			this.columnUsername.AspectName = "VideoInfo.Username";
			this.columnUsername.IsEditable = false;
			this.columnUsername.Text = "Username";
			this.columnUsername.Width = 93;
			// 
			// columnTitle
			// 
			this.columnTitle.AspectName = "VideoInfo.VideoTitle";
			this.columnTitle.IsEditable = false;
			this.columnTitle.Text = "Title";
			this.columnTitle.Width = 205;
			// 
			// columnGame
			// 
			this.columnGame.AspectName = "VideoInfo.VideoGame";
			this.columnGame.IsEditable = false;
			this.columnGame.Text = "Game";
			this.columnGame.Width = 99;
			// 
			// columnTimestamp
			// 
			this.columnTimestamp.AspectName = "VideoInfo.VideoTimestamp";
			this.columnTimestamp.IsEditable = false;
			this.columnTimestamp.Text = "Timestamp";
			this.columnTimestamp.Width = 119;
			// 
			// columnLength
			// 
			this.columnLength.AspectName = "VideoInfo.VideoLength";
			this.columnLength.IsEditable = false;
			this.columnLength.Text = "Duration";
			// 
			// columnRecordingState
			// 
			this.columnRecordingState.AspectName = "VideoInfo.VideoRecordingState";
			this.columnRecordingState.IsEditable = false;
			this.columnRecordingState.Text = "Recording State";
			// 
			// columnNotes
			// 
			this.columnNotes.AspectName = "Notes";
			this.columnNotes.Text = "Notes";
			this.columnNotes.Width = 221;
			// 
			// columnAction1
			// 
			this.columnAction1.AspectName = "ButtonAction";
			this.columnAction1.ButtonSizing = BrightIdeasSoftware.OLVColumn.ButtonSizingMode.CellBounds;
			this.columnAction1.IsButton = true;
			this.columnAction1.IsEditable = false;
			this.columnAction1.Text = "Action";
			// 
			// columnActionRemove
			// 
			this.columnActionRemove.AspectName = "VideoInfo.Service";
			this.columnActionRemove.AspectToStringFormat = "Remove";
			this.columnActionRemove.ButtonSizing = BrightIdeasSoftware.OLVColumn.ButtonSizingMode.CellBounds;
			this.columnActionRemove.IsButton = true;
			this.columnActionRemove.IsEditable = false;
			this.columnActionRemove.Text = "Remove";
			// 
			// columnStatus
			// 
			this.columnStatus.AspectName = "Status";
			this.columnStatus.FillsFreeSpace = true;
			this.columnStatus.IsEditable = false;
			this.columnStatus.Text = "Status";
			this.columnStatus.Width = 50;
			// 
			// buttonSettings
			// 
			this.buttonSettings.BackgroundImage = global::VodArchiver.Properties.Resources.cog;
			this.buttonSettings.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center;
			this.buttonSettings.Location = new System.Drawing.Point(12, 12);
			this.buttonSettings.Name = "buttonSettings";
			this.buttonSettings.Size = new System.Drawing.Size(24, 24);
			this.buttonSettings.TabIndex = 5;
			this.buttonSettings.UseVisualStyleBackColor = true;
			this.buttonSettings.Click += new System.EventHandler(this.buttonSettings_Click);
			// 
			// buttonFetchUser
			// 
			this.buttonFetchUser.Location = new System.Drawing.Point(42, 13);
			this.buttonFetchUser.Name = "buttonFetchUser";
			this.buttonFetchUser.Size = new System.Drawing.Size(117, 20);
			this.buttonFetchUser.TabIndex = 6;
			this.buttonFetchUser.Text = "Fetch User\'s Videos";
			this.buttonFetchUser.UseVisualStyleBackColor = true;
			this.buttonFetchUser.Click += new System.EventHandler(this.buttonFetchUser_Click);
			// 
			// labelStatusBar
			// 
			this.labelStatusBar.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.labelStatusBar.AutoSize = true;
			this.labelStatusBar.Location = new System.Drawing.Point(12, 452);
			this.labelStatusBar.Name = "labelStatusBar";
			this.labelStatusBar.Size = new System.Drawing.Size(0, 13);
			this.labelStatusBar.TabIndex = 7;
			// 
			// labelPowerStateWhenDone
			// 
			this.labelPowerStateWhenDone.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.labelPowerStateWhenDone.AutoSize = true;
			this.labelPowerStateWhenDone.Location = new System.Drawing.Point(1222, 452);
			this.labelPowerStateWhenDone.Name = "labelPowerStateWhenDone";
			this.labelPowerStateWhenDone.Size = new System.Drawing.Size(144, 13);
			this.labelPowerStateWhenDone.TabIndex = 9;
			this.labelPowerStateWhenDone.Text = "when downloads are finished";
			// 
			// comboBoxPowerStateWhenDone
			// 
			this.comboBoxPowerStateWhenDone.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.comboBoxPowerStateWhenDone.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.comboBoxPowerStateWhenDone.FormattingEnabled = true;
			this.comboBoxPowerStateWhenDone.Items.AddRange(new object[] {
            "Do nothing",
            "Close VodArchiver",
            "Sleep",
            "Hibernate"});
			this.comboBoxPowerStateWhenDone.Location = new System.Drawing.Point(1099, 449);
			this.comboBoxPowerStateWhenDone.Name = "comboBoxPowerStateWhenDone";
			this.comboBoxPowerStateWhenDone.Size = new System.Drawing.Size(117, 21);
			this.comboBoxPowerStateWhenDone.TabIndex = 10;
			this.comboBoxPowerStateWhenDone.SelectedIndexChanged += new System.EventHandler(this.comboBoxPowerStateWhenDone_SelectedIndexChanged);
			// 
			// DownloadForm
			// 
			this.AcceptButton = this.buttonDownload;
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(1378, 473);
			this.Controls.Add(this.comboBoxPowerStateWhenDone);
			this.Controls.Add(this.labelPowerStateWhenDone);
			this.Controls.Add(this.labelStatusBar);
			this.Controls.Add(this.buttonFetchUser);
			this.Controls.Add(this.buttonSettings);
			this.Controls.Add(this.objectListViewDownloads);
			this.Controls.Add(this.comboBoxService);
			this.Controls.Add(this.textboxMediaId);
			this.Controls.Add(this.labelMediaId);
			this.Controls.Add(this.buttonDownload);
			this.Name = "DownloadForm";
			this.Text = "VodArchiver";
			this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.DownloadForm_FormClosing);
			((System.ComponentModel.ISupportInitialize)(this.objectListViewDownloads)).EndInit();
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.Button buttonDownload;
		private System.Windows.Forms.Label labelMediaId;
		private System.Windows.Forms.TextBox textboxMediaId;
		private System.Windows.Forms.ComboBox comboBoxService;
		private BrightIdeasSoftware.ObjectListView objectListViewDownloads;
		private BrightIdeasSoftware.OLVColumn columnVideoId;
		private BrightIdeasSoftware.OLVColumn columnUsername;
		private BrightIdeasSoftware.OLVColumn columnService;
		private BrightIdeasSoftware.OLVColumn columnStatus;
		private BrightIdeasSoftware.OLVColumn columnTitle;
		private BrightIdeasSoftware.OLVColumn columnGame;
		private BrightIdeasSoftware.OLVColumn columnTimestamp;
		private BrightIdeasSoftware.OLVColumn columnLength;
		private System.Windows.Forms.Button buttonSettings;
		private System.Windows.Forms.Button buttonFetchUser;
		private BrightIdeasSoftware.OLVColumn columnRecordingState;
		private BrightIdeasSoftware.OLVColumn columnAction1;
		private BrightIdeasSoftware.OLVColumn columnActionRemove;
		private BrightIdeasSoftware.OLVColumn columnIndex;
		private BrightIdeasSoftware.OLVColumn columnValidated;
		private System.Windows.Forms.Label labelStatusBar;
		private BrightIdeasSoftware.OLVColumn columnNotes;
		private System.Windows.Forms.Label labelPowerStateWhenDone;
		private System.Windows.Forms.ComboBox comboBoxPowerStateWhenDone;
	}
}