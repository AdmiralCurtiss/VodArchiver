using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VodArchiver.VideoJobs {
	public interface IUserInputRequest {
		string GetQuestion();
		List<string> GetOptions();
		void SelectOption( string option );
	}
}
