#ifndef HLL_GLOBALS_H__
#define HLL_GLOBALS_H__

#include "maxon/string.h"

namespace HLL
{
	class Globals
	{
		Globals() = delete;
		Globals(const Globals&) = delete;
		Globals& operator=(const Globals&) = delete;

	public:
		struct Version
		{
			const static inline int major = 2;
			const static inline int minor = 1;
			const static inline char* tag = "dev";

#ifdef _DEBUG
			const static inline char* profile = "Debug";
#else
			const static inline char* profile = "Release";
#endif
			static inline String fullString() {
				String v = "v";
				v += String::IntToString(major) + "." + String::IntToString(minor);
				if (std::strlen(tag) > 0) v += "-" + String(tag);
				v += "-" + String(profile);
				return v;
			}
		};

		const static inline int pluginId = 1050016;
		const static inline char* userConfigFile = "userconfig.xml";
		const static inline char* updateLink = "http://code.thatnwp.com/version/HLAELiveLinkUpdate.xml";
	};
}

#endif // !HLL_GLOBALS_H__
