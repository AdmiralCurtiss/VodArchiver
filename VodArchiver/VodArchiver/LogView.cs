﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace VodArchiver {
	public partial class LogView : Form {
		public LogView(string log) {
			InitializeComponent();
			this.textBox1.Text = log;
		}
	}
}
