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
			this.SuspendLayout();
			// 
			// buttonDownload
			// 
			this.buttonDownload.Location = new System.Drawing.Point(174, 12);
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
			this.labelMediaId.Location = new System.Drawing.Point(12, 15);
			this.labelMediaId.Name = "labelMediaId";
			this.labelMediaId.Size = new System.Drawing.Size(50, 13);
			this.labelMediaId.TabIndex = 1;
			this.labelMediaId.Text = "Media ID";
			// 
			// textboxMediaId
			// 
			this.textboxMediaId.Location = new System.Drawing.Point(68, 12);
			this.textboxMediaId.Name = "textboxMediaId";
			this.textboxMediaId.Size = new System.Drawing.Size(100, 20);
			this.textboxMediaId.TabIndex = 2;
			// 
			// comboBoxService
			// 
			this.comboBoxService.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.comboBoxService.FormattingEnabled = true;
			this.comboBoxService.Items.AddRange(new object[] {
            "Twitch",
            "Hitbox"});
			this.comboBoxService.Location = new System.Drawing.Point(12, 39);
			this.comboBoxService.Name = "comboBoxService";
			this.comboBoxService.Size = new System.Drawing.Size(89, 21);
			this.comboBoxService.TabIndex = 3;
			// 
			// Form1
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(284, 261);
			this.Controls.Add(this.comboBoxService);
			this.Controls.Add(this.textboxMediaId);
			this.Controls.Add(this.labelMediaId);
			this.Controls.Add(this.buttonDownload);
			this.Name = "Form1";
			this.Text = "Form1";
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.Button buttonDownload;
		private System.Windows.Forms.Label labelMediaId;
		private System.Windows.Forms.TextBox textboxMediaId;
		private System.Windows.Forms.ComboBox comboBoxService;
	}
}