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
		/// Check if there is an update available for the plugin.
		/// @param[out] link			Assigned the update link for the latest version (or the error message on error).
		/// @param[in] url				The url to a formatted xml file containing the latest info version.
		/// @return Bool				True if there is an update.
		//------------------------------------------------------//		
		Bool CheckForUpdates(String *link, const String &url = Globals::updateLink);

		void Log(String msg);
		void LogError(String msg);

		class UserConfig
		{
		public:
			Int32 checkforupdates;
			String hostname;
			Int32 port;
			Int32 pollrate;
			Bool globalcoords;
			Vector orientation;

			UserConfig() : checkforupdates(-1), hostname("127.0.0.1"), port(31337), pollrate(30),
				globalcoords(false), orientation(0.0f, 0.0f, 0.0f) { }
			UserConfig(const Filename& file);

			void SaveFile(const Filename& file);
			Bool LoadFile(const Filename& file);
		};
	}
}

#endif