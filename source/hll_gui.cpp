// Copyright (c) 2018 Brett Anthony. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "hll_gui.h"

Bool HLL::Gui::CreateLayout()
{
	return LoadDialogResource(DLG_HLL, nullptr, 0);
}

Bool HLL::Gui::InitValues()
{
	// Called before first GUI Draw.
	if (!this->SetString(BTN_LISTEN, GeLoadString(STR_START_LISTEN)))
		return false;

	// to-do: Remember last setting.
	if (!this->SetString(TXT_HOSTNAME, "127.0.0.1"_s))
		return false;

	if (!this->SetString(TXT_PORT, "31337"_s))
		return false;

	// this returns false even if it works, need to investigate...
	this->SetInt32(CBX_COORDINATE_SYSTEM, CBO_Y_UP);

	if (!this->SetString(BTN_MAPPING, "<="_s))
		return false;

	if (!this->SetString(TXT_STATUS, GeLoadString(STR_NO_REPORT)))
		return false;

	if (!this->SetString(TXT_CAMERANAME, GeLoadString(STR_NO_CAMERA)))
		return false;

	return true;
}

Bool HLL::Gui::Command(Int32 id, const BaseContainer &msg)
{
	// Called when GUI is interacted with.
	if (id == BTN_LISTEN)
	{
		// to-do: Implement Server
		String str;
		if (this->GetString(BTN_LISTEN, str))
		{
			if (str == GeLoadString(STR_START_LISTEN))
			{
				if (!this->SetString(BTN_LISTEN, GeLoadString(STR_STOP_LISTEN)))
					return false;
				return true;
			}
			
			if (!this->SetString(BTN_LISTEN, GeLoadString(STR_START_LISTEN)))
				return false;
			return true;
		}
		return false;
	}

	if (id == BTN_SETCAMERA)
	{
		// to-do: Camera Setting
		if (!this->SetString(TXT_CAMERANAME, "Sample Camera Name"_s))
			return false;
		return true;
	}

	if (id == BTN_MAPPING)
	{
		String str;
		if (this->GetString(BTN_MAPPING, str))
		{
			if (str == "<=")
			{
				if (!this->SetString(BTN_MAPPING, "=>"_s))
					return false;
				return true;
			}

			if (!this->SetString(BTN_MAPPING, "<="_s))
				return false;
			return true;
		}
		return false;
	}

	return false;
}

Bool HLL::GuiCommand::Execute(BaseDocument *doc)
{
	// Called when plugin is selected from menu.
	oGui = NewObjClear(HLL::Gui);
	if (!oGui->Open(DLG_TYPE::ASYNC, 1050016))
		return false;

	return true;
}

Bool HLL::RegisterHLL()
{
	HLL::GuiCommand *oGuiCommand = NewObjClear(HLL::GuiCommand);
	return RegisterCommandPlugin(1050016, "HLAELiveLink"_s, 0,
		nullptr, "HelpString"_s, oGuiCommand);
}