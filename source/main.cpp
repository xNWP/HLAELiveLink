// Copyright (c) 2018 Brett Anthony. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "main.h"

Bool PluginStart()
{
	String v = "v" + String::IntToString(HLL_VERSION_MAJOR) + "." + String::IntToString(HLL_VERSION_MINOR);
	if (!HLL::RegisterHLL())
	{

		ApplicationOutput(GeLoadString(STR_FAIL_LOAD_HLL) + v);
		return false;
	}
	else
	{
		ApplicationOutput(GeLoadString(STR_LOADED_HLL) + v);
	}

	Filename userFile(GeGetPluginPath() + "\\" + HLL_USERCONFIG_FILE);
	// Create default config file if it does not exist
	if (!GeFExist(userFile))
	{
		// Create head
		maxon::IoXmlNodeInterface *t = maxon::IoXmlNodeInterface::Alloc(maxon::SourceLocation());
		t->SetName("settings"_s);

		// User Settings
		auto user = t->AddChild("user"_s);

		user->AddChild("checkforupdates"_s);
		user->AddChild("hostname"_s);
		user->AddChild("port"_s);
		user->AddChild("pollrate"_s);

		// Write To File
		maxon::Url u("file:///" + userFile.GetString());
		auto r = maxon::IoXmlParser::WriteDocument(u, t, true, nullptr);
		if (r == maxon::FAILED)
		{
			const maxon::Error e = r.GetError();
			ApplicationOutput(e.GetMessage());
		}
	}

	return true;
}

void PluginEnd()
{
}

Bool PluginMessage(Int32 id, void *data)
{
	if (id == C4DPL_INIT_SYS)
	{
		// Don't start plugin without resource.
		if (!g_resource.Init())
			return false;
		return true;
	}

	return true;
}
