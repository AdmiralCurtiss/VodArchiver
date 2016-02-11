namespace VodArchiver {
	partial class Form1 {
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
			this.olvColumn1 = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.olvColumn2 = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.olvColumn3 = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			this.olvColumn4 = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
			((System.ComponentModel.ISupportInitialize)(this.objectListViewDownloads)).BeginInit();
			this.SuspendLayout();
			// 
			// buttonDownload
			// 
			this.buttonDownload.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonDownload.Location = new System.Drawing.Point(528, 12);
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
			this.labelMediaId.Location = new System.Drawing.Point(113, 15);
			this.labelMediaId.Name = "labelMediaId";
			this.labelMediaId.Size = new System.Drawing.Size(50, 13);
			this.labelMediaId.TabIndex = 1;
			this.labelMediaId.Text = "Media ID";
			// 
			// textboxMediaId
			// 
			this.textboxMediaId.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.textboxMediaId.Location = new System.Drawing.Point(169, 12);
			this.textboxMediaId.Name = "textboxMediaId";
			this.textboxMediaId.Size = new System.Drawing.Size(353, 20);
			this.textboxMediaId.TabIndex = 2;
			// 
			// comboBoxService
			// 
			this.comboBoxService.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.comboBoxService.FormattingEnabled = true;
			this.comboBoxService.Items.AddRange(new object[] {
            "Twitch",
            "Hitbox"});
			this.comboBoxService.Location = new System.Drawing.Point(12, 13);
			this.comboBoxService.Name = "comboBoxService";
			this.comboBoxService.Size = new System.Drawing.Size(89, 21);
			this.comboBoxService.TabIndex = 3;
			// 
			// objectListViewDownloads
			// 
			this.objectListViewDownloads.AllColumns.Add(this.olvColumn3);
			this.objectListViewDownloads.AllColumns.Add(this.olvColumn1);
			this.objectListViewDownloads.AllColumns.Add(this.olvColumn2);
			this.objectListViewDownloads.AllColumns.Add(this.olvColumn4);
			this.objectListViewDownloads.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.objectListViewDownloads.CellEditUseWholeCell = false;
			this.objectListViewDownloads.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.olvColumn3,
            this.olvColumn1,
            this.olvColumn2,
            this.olvColumn4});
			this.objectListViewDownloads.Cursor = System.Windows.Forms.Cursors.Default;
			this.objectListViewDownloads.HighlightBackgroundColor = System.Drawing.Color.Empty;
			this.objectListViewDownloads.HighlightForegroundColor = System.Drawing.Color.Empty;
			this.objectListViewDownloads.Location = new System.Drawing.Point(13, 41);
			this.objectListViewDownloads.Name = "objectListViewDownloads";
			this.objectListViewDownloads.Size = new System.Drawing.Size(590, 282);
			this.objectListViewDownloads.TabIndex = 4;
			this.objectListViewDownloads.UseCompatibleStateImageBehavior = false;
			this.objectListViewDownloads.View = System.Windows.Forms.View.Details;
			// 
			// olvColumn1
			// 
			this.olvColumn1.AspectName = "VideoId";
			this.olvColumn1.Text = "Video ID";
			// 
			// olvColumn2
			// 
			this.olvColumn2.AspectName = "Username";
			this.olvColumn2.Text = "Username";
			// 
			// olvColumn3
			// 
			this.olvColumn3.AspectName = "ServiceName";
			this.olvColumn3.Text = "Service";
			// 
			// olvColumn4
			// 
			this.olvColumn4.AspectName = "Status";
			this.olvColumn4.Text = "Status";
			// 
			// Form1
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(615, 335);
			this.Controls.Add(this.objectListViewDownloads);
			this.Controls.Add(this.comboBoxService);
			this.Controls.Add(this.textboxMediaId);
			this.Controls.Add(this.labelMediaId);
			this.Controls.Add(this.buttonDownload);
			this.Name = "Form1";
			this.Text = "Form1";
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
		private BrightIdeasSoftware.OLVColumn olvColumn1;
		private BrightIdeasSoftware.OLVColumn olvColumn2;
		private BrightIdeasSoftware.OLVColumn olvColumn3;
		private BrightIdeasSoftware.OLVColumn olvColumn4;
	}
}