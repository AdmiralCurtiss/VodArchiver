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
					List<object> localUpdateListDedup = new List<object>();
					foreach ( object o in localUpdateList ) {
						if ( !ContainsInstance( localUpdateListDedup, o ) ) {
							localUpdateListDedup.Add( o );
						}
					}
					ObjectListView.RefreshObjects( localUpdateListDedup );
					localUpdateList.Clear();
				}

				await Task.Delay( 500 );
			}
		}

		private bool ContainsInstance( List<object> list, object o ) {
			foreach ( object l in list ) {
				if ( l == o ) {
					return true;
				}
			}
			return false;
		}

		public void RequestUpdate( object o ) {
			lock ( Lock ) {
				UpdateList.Add( o );
			}
		}
	}
}
