using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

namespace VodArchiver {
	public static class XmlUtil {
		public static XmlAttribute AppendAttribute( this XmlNode node, XmlDocument document, String name, String value ) {
			XmlAttribute attr = document.CreateAttribute( name );
			attr.Value = value;
			node.Attributes.Append( attr );
			return attr;
		}

		public static XmlElement AppendDictionary( this XmlNode node, XmlDocument document, String name, Dictionary<string, string> value ) {
			return AppendDictionary( node, document, name, value, ( string s ) => s, ( string s ) => s );
		}

		public static XmlElement AppendDictionary<T, U>( this XmlNode node, XmlDocument document, String name, Dictionary<T, U> value, Func<T, string> keyToStringMethod, Func<U, string> valueToStringMethod ) {
			XmlElement element = document.CreateElement( name );
			foreach ( var kvp in value ) {
				element.AppendAttribute( document, "_" + keyToStringMethod( kvp.Key ), valueToStringMethod( kvp.Value ) );
			}
			node.AppendChild( element );
			return element;
		}

		public static Dictionary<T, U> DeserializeDictionary<T, U>( this XmlNode node, Func<string, T> keyFromStringMethod, Func<string, U> valueFromStringMethod ) {
			Dictionary<T, U> d = new Dictionary<T, U>();
			foreach ( XmlAttribute attr in node.Attributes ) {
				d.Add( keyFromStringMethod( attr.Name.Substring( 1 ) ), valueFromStringMethod( attr.Value ) );
			}
			return d;
		}
	}
}
