using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver {
	public class ObjectListViewUpdaterTask {
		private BrightIdeasSoftware.ObjectListView ObjectListView;
		private object Lock;
		private List<object> UpdateList;

		private Task Task;

		public ObjectListViewUpdaterTask( BrightIdeasSoftware.ObjectListView listView ) {
			ObjectListView = listView;
			Lock = new object();
			UpdateList = new List<object>();
			Task = Run();
		}

		private async Task Run() {
			List<object> localUpdateList = new List<object>();
			while ( true ) {
				lock ( Lock ) {
					localUpdateList.AddRange( UpdateList );
					UpdateList.Clear();
				}

				if ( localUpdateList.Count > 0 ) {
					ObjectListView.RefreshObjects( localUpdateList );
					localUpdateList.Clear();
				}

				await Task.Delay( 500 );
			}
		}

		public void RequestUpdate( object o ) {
			lock ( Lock ) {
				UpdateList.Add( o );
			}
		}
	}
}
