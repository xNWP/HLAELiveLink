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
	this->SetTimer(1);

	// Init banner
	this->_banner = NewObj(Banner).GetValue();
	if (!this->AttachUserArea(*_banner, IMG_BANNER, USERAREAFLAGS::NONE))
		return false;
	this->_banner->LayoutChanged();
	this->_banner->Redraw();

	// Init default settings
	if (!g_HLL_Init)
	{
		g_HLL_Hostname = "127.0.0.1";
		g_HLL_Port = "31337";
		g_HLL_Mapping = GeLoadString(STR_TARGET_CINEMA4D);
		g_HLL_Camera = GeLoadString(STR_NO_CAMERA);
		g_HLL_Listen = GeLoadString(STR_START_LISTEN);
		g_HLL_ActiveClient = -1;
		g_HLL_PollRate = "30";

		g_HLL_Init = true;
	}

	// Check settings
	Filename userFile(GeGetPluginPath() + "\\" + HLL_USERCONFIG_FILE);
	maxon::Url u("file:///" + userFile.GetString());
	Int er;
	auto xResult = maxon::IoXmlParser::ReadDocument(u, er);

	if (xResult == maxon::FAILED)
	{
		MessageDialog(STR_USER_SETTINGS_ERROR);
	}
	else
	{
		auto xml_map = Tools::CreateMapFromXML(xResult.GetValue());

		// Pull settings
		auto setting = xml_map["settings.user.checkforupdates"_s];
		if (setting->GetValue() == "true")
		{
			g_HLL_CheckForUpdates = true;
			// check for update (we only do this once to avoid pestering the user
			// if they open/close the gui multiple times in a session).
			if (!g_HLL_UpdateChecked)
			{
				String link;
				Bool res = Tools::CheckForUpdates(&link);
				if (!res)
				{
					SetStatusText(5000, GeLoadString(STR_VERSION_UPTODATE));
				}
				else
				{
					if (QuestionDialog(STR_VERSION_OUTDATED))
						GeOpenHTML(link);
				}
				g_HLL_UpdateChecked = true;
			}
		}
		else if (setting->GetValue() == "false")
		{
			g_HLL_CheckForUpdates = false;
		}
		else
		{
			if (QuestionDialog(STR_ASK_AUTO_UPDATE))
			{
				setting->SetValue("true");
				g_HLL_CheckForUpdates = true;
			}
			else
			{
				setting->SetValue("false");
				g_HLL_CheckForUpdates = false;
			}

			// Save Update Question
			maxon::IoXmlParser::WriteDocument(u, xResult.GetValue(), true, nullptr);
		}

		// Hostname
		setting = xml_map["settings.user.hostname"_s];
		if (setting->GetValue().IsPopulated())
			g_HLL_Hostname = setting->GetValue();

		// Port
		setting = xml_map["settings.user.port"_s];
		if (setting->GetValue().IsPopulated())
			g_HLL_Port = setting->GetValue();

		// Pollrate
		setting = xml_map["settings.user.pollrate"_s];
		if (setting->GetValue().IsPopulated())
			g_HLL_PollRate = setting->GetValue();

		// Use Global Coords
		setting = xml_map["settings.user.globalcoords"_s];
		if (setting->GetValue().IsPopulated())
		{
			if (setting->GetValue() == "true")
			{
				g_HLL_UseGlobalCoords = true;
			}
			else
			{
				g_HLL_UseGlobalCoords = false;
			}
		}
	}

	// Create Menu
	MenuFlushAll();

	MenuSubBegin(GeLoadString(STR_SETTINGS));
	MenuAddString(ID_USE_GLOBAL_POSROT, GeLoadString(STR_USE_GLOBAL_POSROT));
	MenuInitString(ID_USE_GLOBAL_POSROT, true, g_HLL_UseGlobalCoords);
	MenuSubEnd();

	MenuSubBegin(GeLoadString(STR_UPDATE));
	MenuAddString(CHECK_FOR_UPDATES, GeLoadString(STR_CHECK_FOR_UPDATES));
	MenuInitString(CHECK_FOR_UPDATES, true, g_HLL_CheckForUpdates);
	MenuAddSeparator();
	MenuAddString(CHECK_FOR_UPDATE_NOW, GeLoadString(STR_CHECK_NOW));
	MenuSubEnd();

	MenuSubBegin(GeLoadString(STR_SUPPORT_HLL));
	MenuAddString(ID_DONATE_PAYPAL, GeLoadString(STR_DONATE_PAYPAL));
	MenuSubEnd();

	MenuFinished();

	if (!this->SetString(BTN_LISTEN, g_HLL_Listen))
		return false;

	if (!this->SetString(TXT_HOSTNAME, g_HLL_Hostname))
		return false;

	if (!this->SetString(TXT_PORT, g_HLL_Port))
		return false;

	if (!this->SetString(TXT_POLLRATE, g_HLL_PollRate))
		return false;
	
	if (!this->SetString(BTN_MAPPING, g_HLL_Mapping))
		return false;

	if (!this->SetString(TXT_CAMERANAME, "<< " + g_HLL_Camera + " >>"))
		return false;

	BaseContainer layout;
	layout.SetInt32('chck', LV_COLUMN_CHECKBOX);
	layout.SetInt32('name', LV_COLUMN_BUTTON);
	layout.SetInt32('disc', LV_COLUMN_BUTTON);
	this->_lvclients.SetLayout(3, layout);

	// Handle the case where GUI is reopened while the server is already running
	if (g_HLL_Listening)
	{
		std::vector<Client> c = g_ServerThread->GetClients();
		BaseContainer data;

		for (size_t i = 0; i < c.size(); i++)
		{
			if (g_HLL_ActiveClient >= 0 && g_HLL_ActiveClient == Int32(i))
				data.SetBool('chck', true);

			data.SetString('name', String(c[i]._name));
			data.SetString('disc', GeLoadString(STR_DISCONNECT));
			this->_lvclients.SetItem(Int32(i), data);
		}
		this->_lvclients.DataChanged();
		this->SetListenGadgets(false);

		// server has been started but not closed yet
		if (!g_ServerThread->Closed())
		{
			this->Enable(BTN_LISTEN, true);
		}
	}

	return true;
}

Bool HLL::Gui::Command(Int32 id, const BaseContainer &msg)
{
	// Called when GUI is interacted with.
	if (id == ID_USE_GLOBAL_POSROT)
	{
		g_HLL_UseGlobalCoords = !g_HLL_UseGlobalCoords;
		MenuInitString(ID_USE_GLOBAL_POSROT, true, g_HLL_UseGlobalCoords);
		return true;
	}

	if (id == CHECK_FOR_UPDATE_NOW)
	{
		String link;
		Bool res = Tools::CheckForUpdates(&link);
		if (!res)
		{
			SetStatusText(5000, GeLoadString(STR_VERSION_UPTODATE));
		}
		else
		{
			if (QuestionDialog(STR_VERSION_OUTDATED))
				GeOpenHTML(link);
		}

		return true;
	}

	if (id == CHECK_FOR_UPDATES)
	{
		g_HLL_CheckForUpdates = !g_HLL_CheckForUpdates;
		MenuInitString(CHECK_FOR_UPDATES, true, g_HLL_CheckForUpdates);
		return true;
	}

	if (id == ID_DONATE_PAYPAL)
	{
		GeOpenHTML("https://www.paypal.me/xNWP"_s);
		return true;
	}

	if (id == BTN_LISTEN)
	{
		if (!g_HLL_Listening)
		{
			// listening could have failed, if it does we will reset.
			g_HLL_Listening = true;
			this->SetListenGadgets(false);
			this->SetStatusText(5000, GeLoadString(STR_STARTING_LISTEN));

			String h, p;
			this->GetString(TXT_HOSTNAME, h);
			this->GetString(TXT_PORT, p);

			// to-do check for valid IPv4 Values
			g_ServerThread = ServerThread::Create(h.GetCStringCopy(), p.ToInt32(Bool())).GetValue();

			// This can fail, we will use an event message from the server to signify whether or not
			// the listening succeeded.
			g_ServerThread.Start();
		}
		else
		{
			g_UpdateCamera.Cancel();
			this->SetString(BTN_LISTEN, GeLoadString(STR_START_LISTEN));
			g_HLL_Listen = GeLoadString(STR_START_LISTEN);
			this->SetListenGadgets(false);
			ApplicationOutput("| HLAELiveLink - " + GeLoadString(STR_LISTEN_SERVER_CLOSED));
			this->SetStatusText(0, GeLoadString(STR_RESTART));
			this->SetStatusText(2000, GeLoadString(STR_LISTEN_SERVER_CLOSED));
			g_ServerThread->CloseServer();
		}

		return true;
	}

	if (id == TXT_HOSTNAME)
	{
		String str;
		if (!this->GetString(TXT_HOSTNAME, str))
			return false;
		g_HLL_Hostname = str;
		return true;
	}

	if (id == TXT_PORT)
	{
		String str;
		if (!this->GetString(TXT_PORT, str))
			return false;
		g_HLL_Port = str;
		return true;
	}

	if (id == TXT_POLLRATE)
	{
		String str;
		if (!this->GetString(TXT_POLLRATE, str))
			return false;
		g_HLL_PollRate = str;
		return true;
	}

	if (id == BTN_SETCAMERA)
	{
		BaseObject *o = GetActiveDocument()->GetActiveObject();
		if (!o)
		{
			SetStatusText(2000, GeLoadString(STR_CAM_NO_SELECT));
			return true;
		}

		if (o->GetType() != Ocamera)
		{
			String m = "'" + o->GetName() + "'" + GeLoadString(STR_NOT_A_CAMERA);
			SetStatusText(2000, m);
			return true;
		}

		if (!this->SetString(TXT_CAMERANAME, "<< " + o->GetName() + " >>"))
			return false;
		g_HLL_Camera = o->GetName();
		g_HLL_OCamera = o;
		return true;
	}

	if (id == BTN_COPY_URI)
	{
		String h, p;
		this->GetString(TXT_HOSTNAME, h);
		this->GetString(TXT_PORT, p);
		CopyToClipboard("mirv_pgl url \"ws://" + h + ":" + p + "\";mirv_pgl start");
	}

	if (id == BTN_MAPPING)
	{
		String str;
		if (this->GetString(BTN_MAPPING, str))
		{
			// Deselect all clients on a mapping switch
			std::vector<Client> c = g_ServerThread->GetClients();
			BaseContainer data;
			for (size_t i = 0; i < c.size(); i++)
			{
				data.SetBool('chck', false);
				data.SetString('name', String(c[i]._name));
				data.SetString('disc', GeLoadString(STR_DISCONNECT));
				this->_lvclients.SetItem(Int32(i), data);
			}
			this->_lvclients.DataChanged();

			// Stop any and all camera updating
			g_UpdateCamera.Cancel();
			if (g_HLL_ActiveClient != -1)
			{
				g_ServerThread->SendClient(g_HLL_ActiveClient, "mirv_input end");
				g_ServerThread->SendClient(g_HLL_ActiveClient, "echo " + GeLoadString(STR_INACTIVE_CLIENT));
				g_ServerThread->SendClient(g_HLL_ActiveClient, "mirv_pgl dataStop");
			}
			g_HLL_ActiveClient = -1;
			this->Enable(TXT_POLLRATE, true);

			if (str == GeLoadString(STR_TARGET_CINEMA4D))
			{
				if (!this->SetString(BTN_MAPPING, GeLoadString(STR_TARGET_CSGO)))
					return false;
				g_HLL_Mapping = GeLoadString(STR_TARGET_CSGO);
				return true;
			}

			if (!this->SetString(BTN_MAPPING, GeLoadString(STR_TARGET_CINEMA4D)))
				return false;
			g_HLL_Mapping = GeLoadString(STR_TARGET_CINEMA4D);
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
			Int32 sid = msg.GetInt32(LV_SIMPLE_ITEM_ID);
			if (!g_HLL_OCamera)
			{
				SetStatusText(4000, GeLoadString(STR_CAM_NO_SELECT));
				// Deselect the client
				std::vector<Client> c = g_ServerThread->GetClients();
				BaseContainer data;
				data.SetBool('chck', false);
				data.SetString('name', String(c[sid]._name));
				data.SetString('disc', GeLoadString(STR_DISCONNECT));
				this->_lvclients.SetItem(Int32(sid), data);
				this->_lvclients.DataChanged();

				return true;
			}			

			// stop sending data to active client (if they exist)
			if (g_HLL_ActiveClient != -1)
			{
				g_UpdateCamera.Cancel();
				g_ServerThread->SendClient(g_HLL_ActiveClient, "echo " + GeLoadString(STR_INACTIVE_CLIENT));
				g_ServerThread->SendClient(g_HLL_ActiveClient, "mirv_pgl dataStop");
				g_ServerThread->SendClient(g_HLL_ActiveClient, "mirv_input end");
			}

			// Check if this is a deselect
			if (sid == g_HLL_ActiveClient)
			{
				this->Enable(TXT_POLLRATE, true);
				g_HLL_ActiveClient = -1;
				return true;
			}

			// Set ActiveClient
			g_HLL_ActiveClient = sid;

			// Deselect Other Clients
			std::vector<Client> c = g_ServerThread->GetClients();
			BaseContainer data;
			for (size_t i = 0; i < c.size(); i++)
			{
				if (Int32(i) == g_HLL_ActiveClient)
					continue;
				data.SetBool('chck', false);
				data.SetString('name', String(c[i]._name));
				data.SetString('disc', GeLoadString(STR_DISCONNECT));
				this->_lvclients.SetItem(Int32(i), data);
			}
			this->_lvclients.DataChanged();

			this->SetStatusText(2000, GeLoadString(STR_ACTIVE_CLIENT) +
				String(c[g_HLL_ActiveClient]._name));

			String o;
			this->GetString(BTN_MAPPING, o);
			if (o == GeLoadString(STR_TARGET_CINEMA4D)) // receive cam data
			{
				g_ServerThread->SendClient(g_HLL_ActiveClient, "echo " + GeLoadString(STR_LIVE_CLIENT));
				g_ServerThread->SendClient(g_HLL_ActiveClient, "mirv_pgl dataStart");
			}
			else // transmit cam data
			{
				auto UpdateCamera = [this]()
				{
					if (!g_HLL_OCamera)
					{
						g_UpdateCamera.Cancel();
						g_HLL_Camera = GeLoadString(STR_NO_CAMERA);
						this->SetString(TXT_CAMERANAME, "<< " + g_HLL_Camera + " >>");
					}
					else
					{
						Vector pos, rot;
						if (g_HLL_UseGlobalCoords)
						{
							auto m = g_HLL_OCamera->GetMg();
							pos = m.off;
							rot = MatrixToHPB(m, ROTATIONORDER::HPB);
						}
						else
						{
							pos = g_HLL_OCamera->GetRelPos();
							rot = g_HLL_OCamera->GetRelRot();
						}

						CameraObject *co = static_cast<CameraObject*>(g_HLL_OCamera);
						GeData ft;
						co->GetParameter(CAMERAOBJECT_FOV, ft, DESCFLAGS_GET::NONE);
						
						Float fov = ft.GetFloat();
						fov = RadToDeg(fov);

						pos = Maths::LHToRHCoordinate(pos);
						pos = Vector(pos.z, pos.x, pos.y);
						rot = Maths::LHToRHRotation(rot);
						// Pitch, Heading, Roll
						rot = Vector(RadToDeg(-rot.y), RadToDeg(-rot.x), RadToDeg(-rot.z));

						g_ServerThread->SendClient(g_HLL_ActiveClient, "mirv_input position "
							+ String::FloatToString(pos.x) + " " + String::FloatToString(pos.y) + " "
							+ String::FloatToString(pos.z));
						g_ServerThread->SendClient(g_HLL_ActiveClient, "mirv_input angles "
							+ String::FloatToString(rot.x) + " " + String::FloatToString(rot.y) + " "
							+ String::FloatToString(rot.z));
						g_ServerThread->SendClient(g_HLL_ActiveClient, "mirv_input fov real "
							+ String::FloatToString(fov));
					}
				};

				this->Enable(TXT_POLLRATE, false);
				Float pR = 1.0f / g_HLL_PollRate.ParseToFloat();

				g_ServerThread->SendClient(g_HLL_ActiveClient, "echo " + GeLoadString(STR_LIVE_CLIENT_R));
				g_ServerThread->SendClient(g_HLL_ActiveClient, "mirv_input camera");
				g_UpdateCamera = maxon::TimerInterface::AddPeriodicTimer
				(maxon::Seconds(pR), UpdateCamera, maxon::JOBQUEUE_CURRENT)
					.GetValue();
			}

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

			SetStatusText(5000, GeLoadString(STR_CLIENT_CONNECT) + "[" + data.GetString('name') + "].");

			return true;
		}

		if (message == HLL_EVMSG_CLIENT_DISCONNECT)
		{
			// Stop updating if disconnect is active client
			if (param == g_HLL_ActiveClient)
			{
				g_UpdateCamera.Cancel();
				this->Enable(TXT_POLLRATE, true);
			}

			Bool bReorder = param == static_cast<Int32>(g_ServerThread->GetClients().size()) ? false : true;
			BaseContainer dat;
			this->_lvclients.GetItem(param, &dat);
			String cname = dat.GetString('name');
			this->_lvclients.RemoveItem(param);

			SetStatusText(5000, GeLoadString(STR_CLIENT_DISCONNECT) + "[" + cname + "].");

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

			if (g_HLL_ActiveClient == param)
				g_HLL_ActiveClient = -1;

			return true;
		}

		if (message == HLL_EVMSG_LISTEN_SUCCESS)
		{
			String h, p;
			this->GetString(TXT_HOSTNAME, h);
			this->GetString(TXT_PORT, p);
			this->SetString(BTN_LISTEN, GeLoadString(STR_STOP_LISTEN));
			g_HLL_Listen = GeLoadString(STR_STOP_LISTEN);
			this->Enable(BTN_LISTEN, true);

			String m = GeLoadString(STR_LISTEN) + h + ":" + p;
			this->SetStatusText(0, m);

			return true;
		}

		if (message == HLL_EVMSG_LISTEN_FAILED)
		{
			// Not possible to reset the listening since C4D doesn't play with uWS well...
			g_HLL_Listening = false;
			this->Enable(BTN_LISTEN, false);

			this->SetStatusText(0, GeLoadString(STR_RESTART));

			MessageDialog(STR_LISTEN_FAILED);

			return true;
		}

		if (message == HLL_EVMSG_DATASTART)
		{
			String s;
			this->GetString(BTN_MAPPING, s);
			if (param != g_HLL_ActiveClient || s == GeLoadString(STR_TARGET_CSGO))
			{
				// Don't update if this is not the client we want or we want to be transmitting data
				BaseContainer dat;
				this->_lvclients.GetItem(param, &dat);
				String cname = dat.GetString('name');
				ApplicationOutput("| HLAELiveLink - " + GeLoadString(STR_BAD_DATASTART) + " [" +
					cname + "]");
				SetStatusText(5000, GeLoadString(STR_BAD_DATASTART) + " [" + cname + "]");

				// Ask them politely (yet firmly) to stop sending data
				g_ServerThread->SendClient(param, "mirv_pgl dataStop");
				g_ServerThread->SendClient(param, "echo " + GeLoadString(STR_BAD_DATASTART_C));

				return true;
			}

			auto UpdateCamera = [this]()
			{
				if (!g_HLL_OCamera)
				{
					g_UpdateCamera.Cancel();
					g_HLL_Camera = GeLoadString(STR_NO_CAMERA);
					this->SetString(TXT_CAMERANAME, "<< " + g_HLL_Camera + " >>");
				}
				else
				{
					Vector pos = Vector(g_ServerThread->GetPositionVec());
					// Left/Right, Up/Down, Forwards/Backwards
					pos = Vector(pos.y, pos.z, pos.x);
					pos = Maths::RHToLHCoordinate(pos);
					Vector rot = Vector(g_ServerThread->GetRotationVec());

					// We will also negate the rotations since c4d uses the opposite rotation direction
					// to a typical left-handed system.
					// Heading, Pitch, Bank
					rot = Vector(DegToRad(-rot.z), DegToRad(-rot.y), DegToRad(-rot.x));
					rot = Maths::RHToLHRotation(rot);

					if (g_HLL_UseGlobalCoords)
					{
						auto m = HPBToMatrix(rot, ROTATIONORDER::HPB);
						m.off = pos;
						g_HLL_OCamera->SetMg(m);
					}
					else
					{
						g_HLL_OCamera->SetRelPos(pos);
						g_HLL_OCamera->SetRelRot(rot);
					}
					
					g_HLL_OCamera->SetParameter(CAMERAOBJECT_FOV, DegToRad(g_ServerThread->GetFov()), DESCFLAGS_SET::NONE);
					maxon::ExecuteOnMainThread([]() {DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_REDUCTION | DRAWFLAGS::STATICBREAK); }, false);
				}
			};

			this->Enable(TXT_POLLRATE, false);
			Float pR = 1.0f / g_HLL_PollRate.ParseToFloat();

			g_UpdateCamera = maxon::TimerInterface::AddPeriodicTimer
			(maxon::Seconds(pR), UpdateCamera, maxon::JOBQUEUE_CURRENT)
				.GetValue();

			return true;
		}

		if (message == HLL_EVMSG_DATASTOP)
		{
			// only cancel if the client that issued dataStop was the active client
			if (param == g_HLL_ActiveClient)
			{
				g_UpdateCamera.Cancel();
				this->Enable(TXT_POLLRATE, true);

				// check if the client issued this dataStop or if it was us, react accordingly.
				BaseContainer dat;
				this->_lvclients.GetItem(param, &dat);

				// it wasn't us
				if (dat.GetBool('chck'))
				{
					String cname = dat.GetString('name');
					ApplicationOutput("| HLAELiveLink - " + GeLoadString(STR_BAD_DATASTOP) + " ["
						+ cname + "]");
					SetStatusText(5000, GeLoadString(STR_BAD_DATASTOP) + " [" + cname + "]");
					// deselect their entry
					dat.SetBool('chck', false);
					this->_lvclients.SetItem(param, dat);
					this->_lvclients.DataChanged();
					g_HLL_ActiveClient = -1;
				}
			}

			return true;
		}
	}

	return false;
}

void HLL::Gui::DestroyWindow()
{
	// Save settings before closing
	Filename userFile(GeGetPluginPath() + "\\" + HLL_USERCONFIG_FILE);
	maxon::Url u("file:///" + userFile.GetString());
	Int er;
	auto xResult = maxon::IoXmlParser::ReadDocument(u, er);

	if (xResult == maxon::FAILED)
	{
		auto error = xResult.GetError();
		MessageDialog("USERCONFIG ERROR: "_s + error.GetMessage());
	}
	else
	{
		auto xml_file = Tools::CreateMapFromXML(xResult.GetValue());

		// Updates
		if (g_HLL_CheckForUpdates)
			xml_file["settings.user.checkforupdates"_s]->SetValue("true");
		else
			xml_file["settings.user.checkforupdates"_s]->SetValue("false");

		// Hostname
		xml_file["settings.user.hostname"_s]->SetValue(g_HLL_Hostname);

		// Port
		xml_file["settings.user.port"_s]->SetValue(g_HLL_Port);

		// Pollrate
		xml_file["settings.user.pollrate"_s]->SetValue(g_HLL_PollRate);

		// Global Coords
		if (g_HLL_UseGlobalCoords)
			xml_file["settings.user.globalcoords"_s]->SetValue("true");
		else
			xml_file["settings.user.globalcoords"_s]->SetValue("false");

		// Write changes
		maxon::IoXmlParser::WriteDocument(u, xResult.GetValue(), true, nullptr);
	}
}

void HLL::Gui::Timer(const BaseContainer &msg)
{
	// set status to message at the top of the queue (LIFO)
	HLL_StatusMessage *a = g_HLL_StatusMessageQueue.GetLast();
	if (!a)
	{
		// no messages
		this->SetString(TXT_STATUS, GeLoadString(STR_NO_REPORT));
		return;
	}
	String m = a->message;

	if (!this->SetString(TXT_STATUS, m))
		return;

	Bool bActive = false;
	// decrement the time for each message
	for (Int32 i = 0; i < g_HLL_StatusMessageQueue.GetCount(); i++)
	{
		// ignore 'sticky' messages
		if (g_HLL_StatusMessageQueue[i].timeleft == 0)
			continue;

		Int32 tl = g_HLL_StatusMessageQueue[i].timeleft - HLL_STATUSBAR_POLL_RATE;
		if (tl <= 0)
			g_HLL_StatusMessageQueue.Erase(i--, 1);
		else
			g_HLL_StatusMessageQueue[i].timeleft = tl;

		bActive = true;
	}

	// stop timer if only sticky messages
	if (!bActive)
	{
		this->SetTimer(0);
		return;
	}
}

void HLL::Gui::SetStatusText(Int32 duration, const String &msg)
{
	HLL_StatusMessage a(duration, msg);
	g_HLL_StatusMessageQueue.Append(a);
	this->SetTimer(HLL_STATUSBAR_POLL_RATE);
}

void HLL::Gui::ClearStatusText()
{
	g_HLL_StatusMessageQueue.Reset();
}

void HLL::Gui::SetListenGadgets(const Bool &val)
{
	this->Enable(BTN_LISTEN, val);
	this->Enable(TXT_HOSTNAME, val);
	this->Enable(TXT_PORT, val);
}

Bool HLL::GuiCommand::Execute(BaseDocument *doc)
{
	// Called when plugin is selected from menu.
	oGui = NewObjClear(HLL::Gui);
	if (!oGui->Open(DLG_TYPE::ASYNC, ID_HLAELIVELINK))
		return false;

	return true;
}

HLL::Gui::Banner::~Banner()
{
	BaseBitmap::Free(bmp);
}

void HLL::Gui::Banner::DrawMsg(Int32 x1, Int32 y1, Int32 x2, Int32 y2, const BaseContainer &msg)
{
	bmp = BaseBitmap::Alloc();
	Filename imagePath(GeGetPluginPath() + "\\res" + "\\banner.png");
	bmp->Init(imagePath);

	DrawBitmap(bmp, 0, 0, bmp->GetBw(), bmp->GetBh(), 0, 0, bmp->GetBw(), bmp->GetBh(), BMP_ALLOWALPHA);
	
	#pragma push_macro("DrawText")
	#undef DrawText
	String v = GeLoadString(STR_VERSION) + " "
		+ String::IntToString(HLL_VERSION_MAJOR) + "."
		+ String::IntToString(HLL_VERSION_MINOR);
	DrawSetTextCol(Vector(1.0f), COLOR_TRANS);
	DrawSetFont(FONT_MONOSPACED);
	DrawText(v, HLL_BANNER_WIDTH - 7, HLL_BANNER_HEIGHT - 9, DRAWTEXT_HALIGN_RIGHT | DRAWTEXT_VALIGN_BOTTOM);
	#pragma pop_macro("DrawText")
}

Bool HLL::RegisterHLL()
{
	BaseBitmap *icon = BaseBitmap::Alloc();
	Filename icoPath(GeGetPluginPath() + "\\res" + "\\icon.png");
	icon->Init(icoPath);
	HLL::GuiCommand *oGuiCommand = NewObjClear(HLL::GuiCommand);
	return RegisterCommandPlugin(ID_HLAELIVELINK, "HLAELiveLink"_s, 0,
		icon, GeLoadString(STR_HELP_STRING), oGuiCommand);
}