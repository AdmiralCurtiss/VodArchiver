namespace VodArchiver {
	partial class AutoRenameWindow {
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
			this.textBoxPath = new System.Windows.Forms.TextBox();
			this.buttonRename = new System.Windows.Forms.Button();
			this.labelPath = new System.Windows.Forms.Label();
			this.buttonGenDurationDiff = new System.Windows.Forms.Button();
			this.textBoxDurationDiff = new System.Windows.Forms.TextBox();
			this.SuspendLayout();
			// 
			// textBoxPath
			// 
			this.textBoxPath.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.textBoxPath.Location = new System.Drawing.Point(47, 12);
			this.textBoxPath.Name = "textBoxPath";
			this.textBoxPath.Size = new System.Drawing.Size(660, 20);
			this.textBoxPath.TabIndex = 0;
			// 
			// buttonRename
			// 
			this.buttonRename.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonRename.Location = new System.Drawing.Point(713, 10);
			this.buttonRename.Name = "buttonRename";
			this.buttonRename.Size = new System.Drawing.Size(75, 23);
			this.buttonRename.TabIndex = 1;
			this.buttonRename.Text = "Rename";
			this.buttonRename.UseVisualStyleBackColor = true;
			this.buttonRename.Click += new System.EventHandler(this.buttonRename_Click);
			// 
			// labelPath
			// 
			this.labelPath.AutoSize = true;
			this.labelPath.Location = new System.Drawing.Point(12, 15);
			this.labelPath.Name = "labelPath";
			this.labelPath.Size = new System.Drawing.Size(29, 13);
			this.labelPath.TabIndex = 2;
			this.labelPath.Text = "Path";
			// 
			// buttonGenDurationDiff
			// 
			this.buttonGenDurationDiff.Location = new System.Drawing.Point(12, 57);
			this.buttonGenDurationDiff.Name = "buttonGenDurationDiff";
			this.buttonGenDurationDiff.Size = new System.Drawing.Size(140, 23);
			this.buttonGenDurationDiff.TabIndex = 3;
			this.buttonGenDurationDiff.Text = "Generate Duration Diff";
			this.buttonGenDurationDiff.UseVisualStyleBackColor = true;
			this.buttonGenDurationDiff.Click += new System.EventHandler(this.buttonGenDurationDiff_Click);
			// 
			// textBoxDurationDiff
			// 
			this.textBoxDurationDiff.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.textBoxDurationDiff.Location = new System.Drawing.Point(12, 86);
			this.textBoxDurationDiff.Multiline = true;
			this.textBoxDurationDiff.Name = "textBoxDurationDiff";
			this.textBoxDurationDiff.ScrollBars = System.Windows.Forms.ScrollBars.Both;
			this.textBoxDurationDiff.Size = new System.Drawing.Size(776, 127);
			this.textBoxDurationDiff.TabIndex = 4;
			// 
			// AutoRenameWindow
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(800, 450);
			this.Controls.Add(this.textBoxDurationDiff);
			this.Controls.Add(this.buttonGenDurationDiff);
			this.Controls.Add(this.labelPath);
			this.Controls.Add(this.buttonRename);
			this.Controls.Add(this.textBoxPath);
			this.Name = "AutoRenameWindow";
			this.Text = "AutoRenameWindow";
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.TextBox textBoxPath;
		private System.Windows.Forms.Button buttonRename;
		private System.Windows.Forms.Label labelPath;
		private System.Windows.Forms.Button buttonGenDurationDiff;
		private System.Windows.Forms.TextBox textBoxDurationDiff;
	}
}