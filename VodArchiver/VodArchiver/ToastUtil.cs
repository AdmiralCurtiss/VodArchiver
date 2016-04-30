using System;
using System.IO;
using Windows.UI.Notifications;
using Windows.Data.Xml.Dom;

namespace VodArchiver {
	public static class ToastUtil {
		public static void ShowToast( string text ) {
			XmlDocument toastXml = ToastNotificationManager.GetTemplateContent( ToastTemplateType.ToastText02 );

			XmlNodeList stringElements = toastXml.GetElementsByTagName( "text" );
			stringElements[0].AppendChild( toastXml.CreateTextNode( "VodArchiver" ) );
			stringElements[1].AppendChild( toastXml.CreateTextNode( text ) );

			ToastNotification toast = new ToastNotification( toastXml );
			ToastNotificationManager.CreateToastNotifier( Util.AppUserModelId ).Show( toast );
		}
	}
}
