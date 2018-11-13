// Copyright (c) 2018 Brett Anthony. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "main.h"

Bool PluginStart()
{
	if (!HLL::RegisterHLL())
	{
		ApplicationOutput("Failed to Load HLL"_s);
		return false;
	}
	else
	{
		ApplicationOutput("Loaded HLL"_s);
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
