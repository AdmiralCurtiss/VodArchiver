using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace VodArchiver {
	public partial class SettingsWindow : Form {
		public SettingsWindow() {
			InitializeComponent();
			textBoxTargetFolder.Text = Util.TargetFolderPath;
			textBoxTempFolder.Text = Util.TempFolderPath;
			checkBoxCustomLocationPersistentData.Checked = Util.EnableCustomDataPath;
			textBoxCustomLocationPersistentData.Text = Util.PersistentDataPath;
		}

		private void buttonSave_Click( object sender, EventArgs e ) {
			Properties.Settings.Default.TargetFolder = textBoxTargetFolder.Text;
			Properties.Settings.Default.TempFolder = textBoxTempFolder.Text;
			Properties.Settings.Default.EnableCustomDataPath = checkBoxCustomLocationPersistentData.Checked;
			Properties.Settings.Default.CustomDataPath = Properties.Settings.Default.EnableCustomDataPath ? textBoxCustomLocationPersistentData.Text : "";
			Properties.Settings.Default.Save();
			this.Close();
		}

		private void buttonCancel_Click( object sender, EventArgs e ) {
			this.Close();
		}

		private void checkBoxCustomLocationPersistentData_CheckedChanged( object sender, EventArgs e ) {
			label3.Enabled = checkBoxCustomLocationPersistentData.Checked;
			textBoxCustomLocationPersistentData.Enabled = checkBoxCustomLocationPersistentData.Checked;
			textBoxCustomLocationPersistentData.ReadOnly = !checkBoxCustomLocationPersistentData.Checked;
		}
	}
}
