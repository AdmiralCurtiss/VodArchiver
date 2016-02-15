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
			this.olvColumn3 = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.olvColumn1 = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.olvColumn2 = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.olvColumn5 = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.olvColumn6 = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.olvColumn7 = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.olvColumn8 = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.downloadButton = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.comboBoxService = new System.Windows.Forms.ComboBox();
			this.textboxUsername = new System.Windows.Forms.TextBox();
			this.labelUsername = new System.Windows.Forms.Label();
			this.buttonDownload = new System.Windows.Forms.Button();
			this.olvColumn4 = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			((System.ComponentModel.ISupportInitialize)(this.objectListViewVideos)).BeginInit();
			this.SuspendLayout();
			// 
			// objectListViewVideos
			// 
			this.objectListViewVideos.AllColumns.Add(this.olvColumn3);
			this.objectListViewVideos.AllColumns.Add(this.olvColumn1);
			this.objectListViewVideos.AllColumns.Add(this.olvColumn2);
			this.objectListViewVideos.AllColumns.Add(this.olvColumn5);
			this.objectListViewVideos.AllColumns.Add(this.olvColumn6);
			this.objectListViewVideos.AllColumns.Add(this.olvColumn7);
			this.objectListViewVideos.AllColumns.Add(this.olvColumn8);
			this.objectListViewVideos.AllColumns.Add(this.olvColumn4);
			this.objectListViewVideos.AllColumns.Add(this.downloadButton);
			this.objectListViewVideos.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.objectListViewVideos.CellEditUseWholeCell = false;
			this.objectListViewVideos.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.olvColumn3,
            this.olvColumn1,
            this.olvColumn2,
            this.olvColumn5,
            this.olvColumn6,
            this.olvColumn7,
            this.olvColumn8,
            this.olvColumn4,
            this.downloadButton});
			this.objectListViewVideos.Cursor = System.Windows.Forms.Cursors.Default;
			this.objectListViewVideos.HighlightBackgroundColor = System.Drawing.Color.Empty;
			this.objectListViewVideos.HighlightForegroundColor = System.Drawing.Color.Empty;
			this.objectListViewVideos.Location = new System.Drawing.Point(12, 38);
			this.objectListViewVideos.Name = "objectListViewVideos";
			this.objectListViewVideos.Size = new System.Drawing.Size(898, 418);
			this.objectListViewVideos.TabIndex = 9;
			this.objectListViewVideos.UseCompatibleStateImageBehavior = false;
			this.objectListViewVideos.View = System.Windows.Forms.View.Details;
			this.objectListViewVideos.ButtonClick += new System.EventHandler<BrightIdeasSoftware.CellClickEventArgs>(this.objectListViewVideos_ButtonClick);
			// 
			// olvColumn3
			// 
			this.olvColumn3.AspectName = "Service";
			this.olvColumn3.IsEditable = false;
			this.olvColumn3.Text = "Service";
			this.olvColumn3.Width = 78;
			// 
			// olvColumn1
			// 
			this.olvColumn1.AspectName = "VideoId";
			this.olvColumn1.Groupable = false;
			this.olvColumn1.IsEditable = false;
			this.olvColumn1.Text = "Video ID";
			this.olvColumn1.Width = 68;
			// 
			// olvColumn2
			// 
			this.olvColumn2.AspectName = "Username";
			this.olvColumn2.IsEditable = false;
			this.olvColumn2.Text = "Username";
			this.olvColumn2.Width = 93;
			// 
			// olvColumn5
			// 
			this.olvColumn5.AspectName = "VideoTitle";
			this.olvColumn5.Groupable = false;
			this.olvColumn5.IsEditable = false;
			this.olvColumn5.Text = "Title";
			this.olvColumn5.Width = 205;
			// 
			// olvColumn6
			// 
			this.olvColumn6.AspectName = "VideoGame";
			this.olvColumn6.IsEditable = false;
			this.olvColumn6.Text = "Game";
			this.olvColumn6.Width = 99;
			// 
			// olvColumn7
			// 
			this.olvColumn7.AspectName = "VideoTimestamp";
			this.olvColumn7.Groupable = false;
			this.olvColumn7.IsEditable = false;
			this.olvColumn7.Text = "Timestamp";
			this.olvColumn7.Width = 119;
			// 
			// olvColumn8
			// 
			this.olvColumn8.AspectName = "VideoLength";
			this.olvColumn8.Groupable = false;
			this.olvColumn8.IsEditable = false;
			this.olvColumn8.Text = "Duration";
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
            "Autodetect",
            "Twitch",
            "Hitbox"});
			this.comboBoxService.Location = new System.Drawing.Point(12, 12);
			this.comboBoxService.Name = "comboBoxService";
			this.comboBoxService.Size = new System.Drawing.Size(89, 21);
			this.comboBoxService.TabIndex = 8;
			// 
			// textboxUsername
			// 
			this.textboxUsername.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.textboxUsername.Location = new System.Drawing.Point(168, 12);
			this.textboxUsername.Name = "textboxUsername";
			this.textboxUsername.Size = new System.Drawing.Size(661, 20);
			this.textboxUsername.TabIndex = 7;
			// 
			// labelUsername
			// 
			this.labelUsername.AutoSize = true;
			this.labelUsername.Location = new System.Drawing.Point(107, 15);
			this.labelUsername.Name = "labelUsername";
			this.labelUsername.Size = new System.Drawing.Size(55, 13);
			this.labelUsername.TabIndex = 6;
			this.labelUsername.Text = "Username";
			// 
			// buttonDownload
			// 
			this.buttonDownload.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonDownload.Location = new System.Drawing.Point(835, 12);
			this.buttonDownload.Name = "buttonDownload";
			this.buttonDownload.Size = new System.Drawing.Size(75, 20);
			this.buttonDownload.TabIndex = 5;
			this.buttonDownload.Text = "Fetch";
			this.buttonDownload.UseVisualStyleBackColor = true;
			this.buttonDownload.Click += new System.EventHandler(this.buttonDownload_Click);
			// 
			// olvColumn4
			// 
			this.olvColumn4.AspectName = "VideoRecordingState";
			this.olvColumn4.IsEditable = false;
			this.olvColumn4.Text = "Recording State";
			// 
			// VodList
			// 
			this.AcceptButton = this.buttonDownload;
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(922, 468);
			this.Controls.Add(this.objectListViewVideos);
			this.Controls.Add(this.comboBoxService);
			this.Controls.Add(this.textboxUsername);
			this.Controls.Add(this.labelUsername);
			this.Controls.Add(this.buttonDownload);
			this.Name = "VodList";
			this.Text = "VodList";
			((System.ComponentModel.ISupportInitialize)(this.objectListViewVideos)).EndInit();
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private BrightIdeasSoftware.ObjectListView objectListViewVideos;
		private BrightIdeasSoftware.OLVColumn olvColumn3;
		private BrightIdeasSoftware.OLVColumn olvColumn1;
		private BrightIdeasSoftware.OLVColumn olvColumn2;
		private BrightIdeasSoftware.OLVColumn olvColumn5;
		private BrightIdeasSoftware.OLVColumn olvColumn6;
		private BrightIdeasSoftware.OLVColumn olvColumn7;
		private BrightIdeasSoftware.OLVColumn olvColumn8;
		private System.Windows.Forms.ComboBox comboBoxService;
		private System.Windows.Forms.TextBox textboxUsername;
		private System.Windows.Forms.Label labelUsername;
		private System.Windows.Forms.Button buttonDownload;
		private BrightIdeasSoftware.OLVColumn downloadButton;
		private BrightIdeasSoftware.OLVColumn olvColumn4;
	}
}