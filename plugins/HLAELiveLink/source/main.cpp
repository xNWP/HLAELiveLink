// Copyright (c) 2018 Brett Anthony. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "hll_server.h"
#include "hll_gui.h"
#include "hll_tools.h"

static std::shared_ptr<HLL::Tools::UserConfig> s_userConfig;

Bool PluginStart()
{
	using namespace HLL;
	
	s_userConfig = std::make_shared<Tools::UserConfig>();
	Filename userFile(GeGetPluginPath() + "\\" + Globals::userConfigFile);
	if (GeFExist(userFile))
		if (!s_userConfig->LoadFile(userFile))
			Tools::LogError(GeLoadString(STR_USER_SETTINGS_ERROR));

	String ver = Globals::Version::fullString();
	if (!RegisterHLL(s_userConfig))
	{
		Tools::LogError(GeLoadString(STR_FAIL_LOAD_HLL, ver));
		return false;
	}

	Tools::Log(GeLoadString(STR_LOADED_HLL, ver));


	return true;
}

void PluginEnd()
{
	using namespace HLL;
	s_userConfig->SaveFile(Globals::userConfigFile);
}

Bool PluginMessage(Int32 id, void *data)
{
	using namespace HLL;
	if (id == C4DPL_INIT_SYS)
	{
		// Don't start plugin without resource.
		if (!g_resource.Init())
			return false;
		return true;
	}
	
	// Called just before PluginEnd() is sent to each plugin
	if (id == C4DPL_ENDACTIVITY)
	{
		// todo: stop update, maybe?

		auto& sThread = ServerThread::GetInstance();
		sThread.CloseServer();
		sThread.Wait(maxon::Seconds(1.0));
	}

	return true;
}
