using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver.StatusUpdate {
	public class ObjectListViewStatusUpdate : IStatusUpdate {
		ObjectListViewUpdaterTask UpdaterTask;
		object Object;

		public ObjectListViewStatusUpdate( ObjectListViewUpdaterTask task, object obj ) {
			UpdaterTask = task;
			Object = obj;
		}

		public void Update() {
			UpdaterTask.RequestUpdate( Object );
		}
	}
}
