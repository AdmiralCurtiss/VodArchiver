using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver.StatusUpdate {
	public class ObjectListViewStatusUpdate : IStatusUpdate {
		BrightIdeasSoftware.ObjectListView ObjectListView;
		object Object;

		public ObjectListViewStatusUpdate( BrightIdeasSoftware.ObjectListView listView, object obj ) {
			ObjectListView = listView;
			Object = obj;
		}

		public void Update() {
			ObjectListView.UpdateObject( Object );
		}
	}
}
