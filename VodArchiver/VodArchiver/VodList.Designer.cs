namespace VodArchiver {
	partial class VodList {
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
			this.objectListViewVideos = new BrightIdeasSoftware.ObjectListView();
			this.columnService = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.columnVideoId = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.columnUsername = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.columnTitle = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.columnGame = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.columnTimestamp = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.columnLength = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.columnRecordingState = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.columnType = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.downloadButton = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.comboBoxService = new System.Windows.Forms.ComboBox();
			this.textboxUsername = new System.Windows.Forms.TextBox();
			this.labelUsername = new System.Windows.Forms.Label();
			this.buttonFetch = new System.Windows.Forms.Button();
			this.comboBoxKnownUsers = new System.Windows.Forms.ComboBox();
			this.buttonDownloadFetched = new System.Windows.Forms.Button();
			((System.ComponentModel.ISupportInitialize)(this.objectListViewVideos)).BeginInit();
			this.SuspendLayout();
			// 
			// objectListViewVideos
			// 
			this.objectListViewVideos.AllColumns.Add(this.columnService);
			this.objectListViewVideos.AllColumns.Add(this.columnVideoId);
			this.objectListViewVideos.AllColumns.Add(this.columnUsername);
			this.objectListViewVideos.AllColumns.Add(this.columnTitle);
			this.objectListViewVideos.AllColumns.Add(this.columnGame);
			this.objectListViewVideos.AllColumns.Add(this.columnTimestamp);
			this.objectListViewVideos.AllColumns.Add(this.columnLength);
			this.objectListViewVideos.AllColumns.Add(this.columnRecordingState);
			this.objectListViewVideos.AllColumns.Add(this.columnType);
			this.objectListViewVideos.AllColumns.Add(this.downloadButton);
			this.objectListViewVideos.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.objectListViewVideos.CellEditUseWholeCell = false;
			this.objectListViewVideos.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnService,
            this.columnVideoId,
            this.columnUsername,
            this.columnTitle,
            this.columnGame,
            this.columnTimestamp,
            this.columnLength,
            this.columnRecordingState,
            this.columnType,
            this.downloadButton});
			this.objectListViewVideos.Cursor = System.Windows.Forms.Cursors.Default;
			this.objectListViewVideos.HighlightBackgroundColor = System.Drawing.Color.Empty;
			this.objectListViewVideos.HighlightForegroundColor = System.Drawing.Color.Empty;
			this.objectListViewVideos.Location = new System.Drawing.Point(12, 38);
			this.objectListViewVideos.Name = "objectListViewVideos";
			this.objectListViewVideos.Size = new System.Drawing.Size(959, 403);
			this.objectListViewVideos.TabIndex = 9;
			this.objectListViewVideos.UseCompatibleStateImageBehavior = false;
			this.objectListViewVideos.View = System.Windows.Forms.View.Details;
			this.objectListViewVideos.ButtonClick += new System.EventHandler<BrightIdeasSoftware.CellClickEventArgs>(this.objectListViewVideos_ButtonClick);
			// 
			// columnService
			// 
			this.columnService.AspectName = "Service";
			this.columnService.IsEditable = false;
			this.columnService.Text = "Service";
			this.columnService.Width = 78;
			// 
			// columnVideoId
			// 
			this.columnVideoId.AspectName = "VideoId";
			this.columnVideoId.IsEditable = false;
			this.columnVideoId.Text = "Video ID";
			this.columnVideoId.Width = 68;
			// 
			// columnUsername
			// 
			this.columnUsername.AspectName = "Username";
			this.columnUsername.IsEditable = false;
			this.columnUsername.Text = "Username";
			this.columnUsername.Width = 93;
			// 
			// columnTitle
			// 
			this.columnTitle.AspectName = "VideoTitle";
			this.columnTitle.IsEditable = false;
			this.columnTitle.Text = "Title";
			this.columnTitle.Width = 205;
			// 
			// columnGame
			// 
			this.columnGame.AspectName = "VideoGame";
			this.columnGame.IsEditable = false;
			this.columnGame.Text = "Game";
			this.columnGame.Width = 99;
			// 
			// columnTimestamp
			// 
			this.columnTimestamp.AspectName = "VideoTimestamp";
			this.columnTimestamp.IsEditable = false;
			this.columnTimestamp.Text = "Timestamp";
			this.columnTimestamp.Width = 119;
			// 
			// columnLength
			// 
			this.columnLength.AspectName = "VideoLength";
			this.columnLength.IsEditable = false;
			this.columnLength.Text = "Duration";
			// 
			// columnRecordingState
			// 
			this.columnRecordingState.AspectName = "VideoRecordingState";
			this.columnRecordingState.IsEditable = false;
			this.columnRecordingState.Text = "Recording State";
			this.columnRecordingState.Width = 90;
			// 
			// columnType
			// 
			this.columnType.AspectName = "VideoType";
			this.columnType.IsEditable = false;
			this.columnType.Text = "Type";
			this.columnType.Width = 49;
			// 
			// downloadButton
			// 
			this.downloadButton.AspectName = "Service";
			this.downloadButton.AspectToStringFormat = "Download";
			this.downloadButton.ButtonSizing = BrightIdeasSoftware.OLVColumn.ButtonSizingMode.CellBounds;
			this.downloadButton.Groupable = false;
			this.downloadButton.IsButton = true;
			this.downloadButton.IsEditable = false;
			this.downloadButton.Sortable = false;
			this.downloadButton.Text = "Download";
			this.downloadButton.Width = 70;
			// 
			// comboBoxService
			// 
			this.comboBoxService.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.comboBoxService.FormattingEnabled = true;
			this.comboBoxService.Items.AddRange(new object[] {
            "Twitch (Recordings)",
            "Twitch (Highlights)",
            "Hitbox"});
			this.comboBoxService.Location = new System.Drawing.Point(250, 12);
			this.comboBoxService.Name = "comboBoxService";
			this.comboBoxService.Size = new System.Drawing.Size(133, 21);
			this.comboBoxService.TabIndex = 8;
			// 
			// textboxUsername
			// 
			this.textboxUsername.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.textboxUsername.Location = new System.Drawing.Point(450, 12);
			this.textboxUsername.Name = "textboxUsername";
			this.textboxUsername.Size = new System.Drawing.Size(357, 20);
			this.textboxUsername.TabIndex = 7;
			// 
			// labelUsername
			// 
			this.labelUsername.AutoSize = true;
			this.labelUsername.Location = new System.Drawing.Point(389, 15);
			this.labelUsername.Name = "labelUsername";
			this.labelUsername.Size = new System.Drawing.Size(55, 13);
			this.labelUsername.TabIndex = 6;
			this.labelUsername.Text = "Username";
			// 
			// buttonFetch
			// 
			this.buttonFetch.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonFetch.Location = new System.Drawing.Point(813, 12);
			this.buttonFetch.Name = "buttonFetch";
			this.buttonFetch.Size = new System.Drawing.Size(158, 20);
			this.buttonFetch.TabIndex = 5;
			this.buttonFetch.Text = "Fetch";
			this.buttonFetch.UseVisualStyleBackColor = true;
			this.buttonFetch.Click += new System.EventHandler(this.buttonFetch_Click);
			// 
			// comboBoxKnownUsers
			// 
			this.comboBoxKnownUsers.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.comboBoxKnownUsers.FormattingEnabled = true;
			this.comboBoxKnownUsers.Location = new System.Drawing.Point(12, 12);
			this.comboBoxKnownUsers.Name = "comboBoxKnownUsers";
			this.comboBoxKnownUsers.Size = new System.Drawing.Size(232, 21);
			this.comboBoxKnownUsers.TabIndex = 10;
			this.comboBoxKnownUsers.SelectedIndexChanged += new System.EventHandler(this.comboBoxKnownUsers_SelectedIndexChanged);
			// 
			// buttonDownloadFetched
			// 
			this.buttonDownloadFetched.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonDownloadFetched.Location = new System.Drawing.Point(813, 447);
			this.buttonDownloadFetched.Name = "buttonDownloadFetched";
			this.buttonDownloadFetched.Size = new System.Drawing.Size(158, 23);
			this.buttonDownloadFetched.TabIndex = 11;
			this.buttonDownloadFetched.Text = "Download Fetched";
			this.buttonDownloadFetched.UseVisualStyleBackColor = true;
			this.buttonDownloadFetched.Click += new System.EventHandler(this.buttonDownloadFetched_Click);
			// 
			// VodList
			// 
			this.AcceptButton = this.buttonFetch;
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(983, 482);
			this.Controls.Add(this.buttonDownloadFetched);
			this.Controls.Add(this.comboBoxKnownUsers);
			this.Controls.Add(this.objectListViewVideos);
			this.Controls.Add(this.comboBoxService);
			this.Controls.Add(this.textboxUsername);
			this.Controls.Add(this.labelUsername);
			this.Controls.Add(this.buttonFetch);
			this.Name = "VodList";
			this.Text = "VodList";
			((System.ComponentModel.ISupportInitialize)(this.objectListViewVideos)).EndInit();
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private BrightIdeasSoftware.ObjectListView objectListViewVideos;
		private BrightIdeasSoftware.OLVColumn columnService;
		private BrightIdeasSoftware.OLVColumn columnVideoId;
		private BrightIdeasSoftware.OLVColumn columnUsername;
		private BrightIdeasSoftware.OLVColumn columnTitle;
		private BrightIdeasSoftware.OLVColumn columnGame;
		private BrightIdeasSoftware.OLVColumn columnTimestamp;
		private BrightIdeasSoftware.OLVColumn columnLength;
		private System.Windows.Forms.ComboBox comboBoxService;
		private System.Windows.Forms.TextBox textboxUsername;
		private System.Windows.Forms.Label labelUsername;
		private System.Windows.Forms.Button buttonFetch;
		private BrightIdeasSoftware.OLVColumn downloadButton;
		private BrightIdeasSoftware.OLVColumn columnRecordingState;
		private BrightIdeasSoftware.OLVColumn columnType;
		private System.Windows.Forms.ComboBox comboBoxKnownUsers;
		private System.Windows.Forms.Button buttonDownloadFetched;
	}
}