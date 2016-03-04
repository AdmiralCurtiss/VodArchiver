using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver {
	// terrible name but I can't think of a good one
	public enum ServiceVideoCategoryType {
		TwitchRecordings,
		TwitchHighlights,
		HitboxRecordings
	}

	public class UserInfo : IEquatable<UserInfo>, IEqualityComparer<UserInfo> {
		public ServiceVideoCategoryType Service;
		public string Username;

		public bool Equals( UserInfo other ) {
			return this.Service == other.Service && this.Username == other.Username;
		}

		public bool Equals( UserInfo x, UserInfo y ) {
			return x.Equals( y );
		}

		public int GetHashCode( UserInfo obj ) {
			return obj.GetHashCode();
		}

		public override string ToString() {
			return Service + "|" + Username;
		}

		public override int GetHashCode() {
			return this.ToString().GetHashCode();
		}
	}
}
