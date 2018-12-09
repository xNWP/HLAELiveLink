// Copyright (c) 2018 Brett Anthony. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#ifndef HLL_TOOLS_H__
#define HLL_TOOLS_H__

#include "c4d.h"
#include "hll_globals.h"
#include "maxon/ioxmlparser.h"
#include <vector>

namespace HLL
{
	namespace Tools
	{
		//------------------------------------------------------//
		/// Functionally works the same as std::unordered_map but specific to our case.
		//------------------------------------------------------//
		class XmlUnorderedMap
		{
		public:
			void Emplace(maxon::String key, maxon::IoXmlNodeInterface* val);
			void Emplace(XmlUnorderedMap map);
			maxon::IoXmlNodeInterface* operator[] (const maxon::String &key);

		private:
			std::vector<maxon::String> _keys;
			std::vector<maxon::IoXmlNodeInterface*> _values;
		};

		//------------------------------------------------------//
		/// Looks through a tree, then returns an unordered_map of tree.
		/// For example a node with the name 'int' and value '7' that is a child node of
		/// 'numbers' which is itself a subset of 'MyTree' can be accessed like this:
		/// ReturnMap["MyTree.numbers.int"].GetValue() == "7" (true)
		/// @param[in] xmlHead													The head of the tree.
		/// @return std::unordered_map<String, maxon::IoXmlNodeInterface*>		The map.
		//------------------------------------------------------//
		HLL::Tools::XmlUnorderedMap CreateMapFromXML(maxon::IoXmlNodeInterface * xmlHead);

		//------------------------------------------------------//
		/// Check if there is an update available for the plugin.
		/// @param[out] link			Assigned the update link for the latest version (or the error message on error).
		/// @param[in] url				The url to a formatted xml file containing the latest info version.
		/// @return Bool				True if there is an update.
		//------------------------------------------------------//		
		Bool CheckForUpdates(String *link, const String &url = HLL_UPDATE_LINK);
	}
}

#endif