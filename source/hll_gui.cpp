// Copyright (c) 2018 Brett Anthony. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "hll_gui.h"

Bool HLL::Gui::CreateLayout()
{
	Bool res = GeDialog::CreateLayout();
	res = LoadDialogResource(DLG_HLL, nullptr, 0);
	if (res)
	{
		this->_lvclients.AttachListView(this, LV_CLIENTS);
	}
	return res;
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

	// this returns false even if it works, confirmed bug by Maxon.
	this->SetInt32(CBX_COORDINATE_SYSTEM, CBO_Y_UP);

	if (!this->SetString(BTN_MAPPING, "<="_s))
		return false;

	if (!this->SetString(TXT_STATUS, GeLoadString(STR_NO_REPORT)))
		return false;

	if (!this->SetString(TXT_CAMERANAME, GeLoadString(STR_NO_CAMERA)))
		return false;

	// Attach ListView for clients
	Bool res = this->_lvclients.AttachListView(this, LV_CLIENTS);

	BaseContainer layout;
	layout.SetInt32('chck', LV_COLUMN_CHECKBOX);
	layout.SetInt32('name', LV_COLUMN_BUTTON);
	layout.SetInt32('disc', LV_COLUMN_BUTTON);
	res = this->_lvclients.SetLayout(3, layout);

	return true;
}

Bool HLL::Gui::Command(Int32 id, const BaseContainer &msg)
{
	// Called when GUI is interacted with.
	if (id == BTN_LISTEN)
	{
		DiagnosticOutput("START");
		g_SimpleThread = SimpleThread::Create().GetValue();
		g_SimpleThread.Start();
		DiagnosticOutput("CLEAR");

		return true;
	}

	if (id == BTN_SETCAMERA)
	{
		// to-do: Camera Setting
		if (!this->SetString(TXT_CAMERANAME, "Sample Camera Name"_s))
			return false;

		DiagnosticOutput("SERVER STOP");
		g_SimpleThread.CancelAndWait();
		g_SimpleThread = nullptr; // ERROR
		DiagnosticOutput("CLEAR");

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

	if (id == LV_CLIENTS)
	{
		Int32 action, item, col;
		BaseContainer colData;
		action = msg.GetInt32(BFM_ACTION_VALUE);
		
		if (action == LV_SIMPLE_CHECKBOXCHANGED)
		{
			// to-do: checkbox controls.
			return true;
		}

		if (action == LV_SIMPLE_BUTTONCLICK)
		{
			item = msg.GetInt32(LV_SIMPLE_ITEM_ID);
			col = msg.GetInt32(LV_SIMPLE_COL_ID);

			// rename
			if (col == 'name')
			{
				if (!this->_lvclients.GetItem(item, &colData))
				{
					ApplicationOutput(GeLoadString(STR_UNEXPECTED_ERROR) + "hll_gui.cpp LN 149");
					return false;
				}

				String str = colData.GetString('name');
				if (RenameDialog(&str))
				{
					colData.SetString('name', str);
					this->_lvclients.SetItem(item, colData);
					this->_lvclients.DataChanged();

					g_ServerThread->RenameClient(item, str.GetCStringCopy());

					return true;
				}
				return true;
			}

			// disconnect
			if (col == 'disc')
			{
				g_ServerThread->DisconnectClient(item);
				return true;
			}
		}
	}

	return false;
}

Bool HLL::Gui::CoreMessage(Int32 id, const BaseContainer &msg)
{
	// Catch messages from server to us.
	if (id == ID_HLAELIVELINK)
	{
		Int32 message = (Int32)reinterpret_cast<intptr_t>(msg.GetVoid(BFM_CORE_PAR1));
		Int32 param = (Int32)reinterpret_cast<intptr_t>(msg.GetVoid(BFM_CORE_PAR2));

		if (message == HLL_EVMSG_CLIENT_CONNECT)
		{
			BaseContainer data;
			data.SetString('name', String(g_ServerThread->GetClients()[param]._name));
			data.SetString('disc', GeLoadString(STR_DISCONNECT));
			this->_lvclients.SetItem(param, data);
			this->_lvclients.DataChanged();

			return true;
		}

		if (message == HLL_EVMSG_CLIENT_DISCONNECT)
		{
			Bool bReorder = param == static_cast<Int32>(g_ServerThread->GetClients().size()) ? false : true;
			this->_lvclients.RemoveItem(param);

			if (bReorder)
			{
				BaseContainer data;
				for (Int32 i = param; i < static_cast<Int32>(g_ServerThread->GetClients().size()); i++)
				{
					this->_lvclients.GetItem(i + 1, &data);
					this->_lvclients.SetItem(i, data);
					this->_lvclients.RemoveItem(i + 1);
					data = BaseContainer();
				}
			}

			this->_lvclients.DataChanged();

			return true;
		}
	}

	return true;
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
	return RegisterCommandPlugin(ID_HLAELIVELINK, "HLAELiveLink"_s, 0,
		nullptr, "HelpString"_s, oGuiCommand);
}