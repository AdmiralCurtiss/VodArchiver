namespace VodArchiver {
	partial class SettingsWindow {
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
			this.label1 = new System.Windows.Forms.Label();
			this.textBoxTargetFolder = new System.Windows.Forms.TextBox();
			this.textBoxTempFolder = new System.Windows.Forms.TextBox();
			this.label2 = new System.Windows.Forms.Label();
			this.buttonSave = new System.Windows.Forms.Button();
			this.buttonCancel = new System.Windows.Forms.Button();
			this.checkBoxCustomLocationPersistentData = new System.Windows.Forms.CheckBox();
			this.textBoxCustomLocationPersistentData = new System.Windows.Forms.TextBox();
			this.label3 = new System.Windows.Forms.Label();
			this.textBoxTwitchRedirectURI = new System.Windows.Forms.TextBox();
			this.label4 = new System.Windows.Forms.Label();
			this.textBoxTwitchClientID = new System.Windows.Forms.TextBox();
			this.label5 = new System.Windows.Forms.Label();
			this.numericUpDownMinimumFreeSpace = new System.Windows.Forms.NumericUpDown();
			this.label6 = new System.Windows.Forms.Label();
			this.label7 = new System.Windows.Forms.Label();
			this.label8 = new System.Windows.Forms.Label();
			this.textBoxYoutubeSpeedLimit = new System.Windows.Forms.TextBox();
			this.textBoxTwitchClientSecret = new System.Windows.Forms.TextBox();
			this.label9 = new System.Windows.Forms.Label();
			((System.ComponentModel.ISupportInitialize)(this.numericUpDownMinimumFreeSpace)).BeginInit();
			this.SuspendLayout();
			// 
			// label1
			// 
			this.label1.AutoSize = true;
			this.label1.Location = new System.Drawing.Point(12, 15);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(87, 13);
			this.label1.TabIndex = 0;
			this.label1.Text = "Download Folder";
			// 
			// textBoxTargetFolder
			// 
			this.textBoxTargetFolder.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.textBoxTargetFolder.Location = new System.Drawing.Point(131, 12);
			this.textBoxTargetFolder.Name = "textBoxTargetFolder";
			this.textBoxTargetFolder.Size = new System.Drawing.Size(360, 20);
			this.textBoxTargetFolder.TabIndex = 1;
			// 
			// textBoxTempFolder
			// 
			this.textBoxTempFolder.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.textBoxTempFolder.Location = new System.Drawing.Point(131, 38);
			this.textBoxTempFolder.Name = "textBoxTempFolder";
			this.textBoxTempFolder.Size = new System.Drawing.Size(360, 20);
			this.textBoxTempFolder.TabIndex = 3;
			// 
			// label2
			// 
			this.label2.AutoSize = true;
			this.label2.Location = new System.Drawing.Point(12, 41);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(108, 13);
			this.label2.TabIndex = 2;
			this.label2.Text = "Temp Folder for Parts";
			// 
			// buttonSave
			// 
			this.buttonSave.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonSave.Location = new System.Drawing.Point(390, 284);
			this.buttonSave.Name = "buttonSave";
			this.buttonSave.Size = new System.Drawing.Size(101, 23);
			this.buttonSave.TabIndex = 4;
			this.buttonSave.Text = "Save";
			this.buttonSave.UseVisualStyleBackColor = true;
			this.buttonSave.Click += new System.EventHandler(this.buttonSave_Click);
			// 
			// buttonCancel
			// 
			this.buttonCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonCancel.Location = new System.Drawing.Point(283, 284);
			this.buttonCancel.Name = "buttonCancel";
			this.buttonCancel.Size = new System.Drawing.Size(101, 23);
			this.buttonCancel.TabIndex = 5;
			this.buttonCancel.Text = "Cancel";
			this.buttonCancel.UseVisualStyleBackColor = true;
			this.buttonCancel.Click += new System.EventHandler(this.buttonCancel_Click);
			// 
			// checkBoxCustomLocationPersistentData
			// 
			this.checkBoxCustomLocationPersistentData.AutoSize = true;
			this.checkBoxCustomLocationPersistentData.Location = new System.Drawing.Point(15, 74);
			this.checkBoxCustomLocationPersistentData.Name = "checkBoxCustomLocationPersistentData";
			this.checkBoxCustomLocationPersistentData.Size = new System.Drawing.Size(278, 17);
			this.checkBoxCustomLocationPersistentData.TabIndex = 6;
			this.checkBoxCustomLocationPersistentData.Text = "Custom Location for Persistent Download && User Lists";
			this.checkBoxCustomLocationPersistentData.UseVisualStyleBackColor = true;
			this.checkBoxCustomLocationPersistentData.CheckedChanged += new System.EventHandler(this.checkBoxCustomLocationPersistentData_CheckedChanged);
			// 
			// textBoxCustomLocationPersistentData
			// 
			this.textBoxCustomLocationPersistentData.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.textBoxCustomLocationPersistentData.Enabled = false;
			this.textBoxCustomLocationPersistentData.Location = new System.Drawing.Point(131, 94);
			this.textBoxCustomLocationPersistentData.Name = "textBoxCustomLocationPersistentData";
			this.textBoxCustomLocationPersistentData.Size = new System.Drawing.Size(360, 20);
			this.textBoxCustomLocationPersistentData.TabIndex = 8;
			// 
			// label3
			// 
			this.label3.AutoSize = true;
			this.label3.Enabled = false;
			this.label3.Location = new System.Drawing.Point(12, 97);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(86, 13);
			this.label3.TabIndex = 7;
			this.label3.Text = "Custom Location";
			// 
			// textBoxTwitchRedirectURI
			// 
			this.textBoxTwitchRedirectURI.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.textBoxTwitchRedirectURI.Location = new System.Drawing.Point(131, 182);
			this.textBoxTwitchRedirectURI.Name = "textBoxTwitchRedirectURI";
			this.textBoxTwitchRedirectURI.Size = new System.Drawing.Size(360, 20);
			this.textBoxTwitchRedirectURI.TabIndex = 12;
			// 
			// label4
			// 
			this.label4.AutoSize = true;
			this.label4.Location = new System.Drawing.Point(12, 185);
			this.label4.Name = "label4";
			this.label4.Size = new System.Drawing.Size(104, 13);
			this.label4.TabIndex = 11;
			this.label4.Text = "Twitch Redirect URI";
			// 
			// textBoxTwitchClientID
			// 
			this.textBoxTwitchClientID.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.textBoxTwitchClientID.Location = new System.Drawing.Point(131, 130);
			this.textBoxTwitchClientID.Name = "textBoxTwitchClientID";
			this.textBoxTwitchClientID.Size = new System.Drawing.Size(360, 20);
			this.textBoxTwitchClientID.TabIndex = 10;
			// 
			// label5
			// 
			this.label5.AutoSize = true;
			this.label5.Location = new System.Drawing.Point(12, 133);
			this.label5.Name = "label5";
			this.label5.Size = new System.Drawing.Size(82, 13);
			this.label5.TabIndex = 9;
			this.label5.Text = "Twitch Client ID";
			// 
			// numericUpDownMinimumFreeSpace
			// 
			this.numericUpDownMinimumFreeSpace.Increment = new decimal(new int[] {
            1073741824,
            0,
            0,
            0});
			this.numericUpDownMinimumFreeSpace.Location = new System.Drawing.Point(131, 209);
			this.numericUpDownMinimumFreeSpace.Maximum = new decimal(new int[] {
            1879048191,
            -1291644761,
            -500259693,
            0});
			this.numericUpDownMinimumFreeSpace.Name = "numericUpDownMinimumFreeSpace";
			this.numericUpDownMinimumFreeSpace.Size = new System.Drawing.Size(142, 20);
			this.numericUpDownMinimumFreeSpace.TabIndex = 13;
			// 
			// label6
			// 
			this.label6.AutoSize = true;
			this.label6.Location = new System.Drawing.Point(12, 211);
			this.label6.Name = "label6";
			this.label6.Size = new System.Drawing.Size(106, 13);
			this.label6.TabIndex = 14;
			this.label6.Text = "Minimum Free Space";
			// 
			// label7
			// 
			this.label7.AutoSize = true;
			this.label7.Location = new System.Drawing.Point(279, 211);
			this.label7.Name = "label7";
			this.label7.Size = new System.Drawing.Size(33, 13);
			this.label7.TabIndex = 15;
			this.label7.Text = "Bytes";
			// 
			// label8
			// 
			this.label8.AutoSize = true;
			this.label8.Location = new System.Drawing.Point(12, 238);
			this.label8.Name = "label8";
			this.label8.Size = new System.Drawing.Size(105, 13);
			this.label8.TabIndex = 16;
			this.label8.Text = "Youtube Speed Limit";
			// 
			// textBoxYoutubeSpeedLimit
			// 
			this.textBoxYoutubeSpeedLimit.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.textBoxYoutubeSpeedLimit.Location = new System.Drawing.Point(131, 235);
			this.textBoxYoutubeSpeedLimit.Name = "textBoxYoutubeSpeedLimit";
			this.textBoxYoutubeSpeedLimit.Size = new System.Drawing.Size(360, 20);
			this.textBoxYoutubeSpeedLimit.TabIndex = 17;
			// 
			// textBoxTwitchClientSecret
			// 
			this.textBoxTwitchClientSecret.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.textBoxTwitchClientSecret.Location = new System.Drawing.Point(131, 156);
			this.textBoxTwitchClientSecret.Name = "textBoxTwitchClientSecret";
			this.textBoxTwitchClientSecret.Size = new System.Drawing.Size(360, 20);
			this.textBoxTwitchClientSecret.TabIndex = 19;
			// 
			// label9
			// 
			this.label9.AutoSize = true;
			this.label9.Location = new System.Drawing.Point(12, 159);
			this.label9.Name = "label9";
			this.label9.Size = new System.Drawing.Size(102, 13);
			this.label9.TabIndex = 18;
			this.label9.Text = "Twitch Client Secret";
			// 
			// SettingsWindow
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(503, 319);
			this.Controls.Add(this.textBoxTwitchClientSecret);
			this.Controls.Add(this.label9);
			this.Controls.Add(this.textBoxYoutubeSpeedLimit);
			this.Controls.Add(this.label8);
			this.Controls.Add(this.label7);
			this.Controls.Add(this.label6);
			this.Controls.Add(this.numericUpDownMinimumFreeSpace);
			this.Controls.Add(this.textBoxTwitchRedirectURI);
			this.Controls.Add(this.label4);
			this.Controls.Add(this.textBoxTwitchClientID);
			this.Controls.Add(this.label5);
			this.Controls.Add(this.textBoxCustomLocationPersistentData);
			this.Controls.Add(this.label3);
			this.Controls.Add(this.checkBoxCustomLocationPersistentData);
			this.Controls.Add(this.buttonCancel);
			this.Controls.Add(this.buttonSave);
			this.Controls.Add(this.textBoxTempFolder);
			this.Controls.Add(this.label2);
			this.Controls.Add(this.textBoxTargetFolder);
			this.Controls.Add(this.label1);
			this.Name = "SettingsWindow";
			this.Text = "SettingsWindow";
			((System.ComponentModel.ISupportInitialize)(this.numericUpDownMinimumFreeSpace)).EndInit();
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.TextBox textBoxTargetFolder;
		private System.Windows.Forms.TextBox textBoxTempFolder;
		private System.Windows.Forms.Label label2;
		private System.Windows.Forms.Button buttonSave;
		private System.Windows.Forms.Button buttonCancel;
		private System.Windows.Forms.CheckBox checkBoxCustomLocationPersistentData;
		private System.Windows.Forms.TextBox textBoxCustomLocationPersistentData;
		private System.Windows.Forms.Label label3;
		private System.Windows.Forms.TextBox textBoxTwitchRedirectURI;
		private System.Windows.Forms.Label label4;
		private System.Windows.Forms.TextBox textBoxTwitchClientID;
		private System.Windows.Forms.Label label5;
		private System.Windows.Forms.NumericUpDown numericUpDownMinimumFreeSpace;
		private System.Windows.Forms.Label label6;
		private System.Windows.Forms.Label label7;
		private System.Windows.Forms.Label label8;
		private System.Windows.Forms.TextBox textBoxYoutubeSpeedLimit;
		private System.Windows.Forms.TextBox textBoxTwitchClientSecret;
		private System.Windows.Forms.Label label9;
	}
}