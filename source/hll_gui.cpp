// Copyright (c) 2018 Brett Anthony. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "hll_gui.h"
#include "c4d_general.h"
#include <deque>

namespace HLL
{
	struct StatusMessage
	{
		StatusMessage(Int32 duration, const String &msg) : timeleft(duration), message(msg) { }
		Int32 timeleft;
		String message;
	};

	static maxon::BaseArray<StatusMessage> s_StatusMessageQueue;

	struct GuiData
	{
		String cameraName = ""_s;
		BaseObject* cameraObj = nullptr;
		Bool listening = false;
		Int32 activeClient = -1;
		Bool updating = false;
		String mapping = GeLoadString(STR_TARGET_CINEMA4D);
	};

	static std::unique_ptr<GuiData> s_guiData;
	static std::shared_ptr<Tools::UserConfig> s_userConfig;
	static maxon::TimerRef s_updateCameraTimer;

	enum class LV_CLIENTS_ID : maxon::Int32
	{
		CHCK = 0x0A0B0C0D,
		NAME,
		DISC
	};

	template <typename SampleType, size_t TimeWindowMs>
	class TimeWindowRollingAverage
	{
		struct SampleData
		{
			SampleData(const maxon::TimeValue& t, const SampleType& s) : time(t), sample(s) { }
			maxon::TimeValue time;
			SampleType sample;
		};

		std::deque<SampleData> _samples;
		SampleType _total;

	public:
		TimeWindowRollingAverage() : _samples(std::deque<SampleData>()), _total(SampleType()) { }

		TimeWindowRollingAverage& operator()(const SampleType& sample)
		{
			_total += sample;
			_samples.emplace_back(maxon::TimeValue::GetTime(), sample);
			return *this;
		}

		void Reset() { *this = TimeWindowRollingAverage(); }

		operator SampleType()
		{
			if (_samples.size())
			{
				const auto& last = _samples.back();
				while ((last.time - _samples.front().time).GetMilliseconds() > TimeWindowMs)
				{
					_total -= _samples.front().sample;
					_samples.pop_front();
				}
			}
			else
			{
				return SampleType();
			}

			return _total / _samples.size();
		}
	};

	template <size_t TimeWindowMs>
	class TimeWindowCount
	{
		std::deque<maxon::TimeValue> _counts;

	public:
		TimeWindowCount() : _counts(std::deque<maxon::TimeValue>()) { };

		void Reset() { *this = TimeWindowCount(); }

		void Count() { _counts.emplace_back(maxon::TimeValue::GetTime()); }

		size_t GetCounts()
		{
			if (_counts.size())
			{
				const auto& last = _counts.back();
				while ((last - _counts.front()).GetMilliseconds() > TimeWindowMs)
					_counts.pop_front();
			}

			return _counts.size();
		}
	};

	Bool Gui::StartUpdate(Bool setup_client)
	{
		if (!IsCameraValid())
		{
			ResetCamera();
			return false;
		}

		static std::function<void(void)> UpdateCamera;

		Bool transmit = s_guiData->mapping == GeLoadString(STR_TARGET_CSGO);
		static TimeWindowRollingAverage<maxon::TimeValue, 1500> TimerAverage;
		TimerAverage.Reset();

		auto& sThread = ServerThread::GetInstance();
		if (transmit)
		{
			// We are transmit, client is receive
			UpdateCamera = [&]()
			{
				maxon::TimeValue Timer = maxon::TimeValue::GetTime();

				maxon::Matrix64 m;
				if (s_userConfig->globalcoords) m = HPBToMatrix(s_userConfig->orientation, ROTATIONORDER::HPB) * s_guiData->cameraObj->GetMg();
				else m = HPBToMatrix(s_userConfig->orientation, ROTATIONORDER::HPB) * s_guiData->cameraObj->GetMl();
				Vector pos = m.off;
				Vector rot = MatrixToHPB(m, ROTATIONORDER::HPB);

				CameraObject *co = static_cast<CameraObject*>(s_guiData->cameraObj);

				GeData ft;
				co->GetParameter(CAMERAOBJECT_FOV, ft, DESCFLAGS_GET::NONE);

				Float fov = ft.GetFloat();
				fov = RadToDeg(fov);

				pos = Maths::LHToRHCoordinate(pos);
				pos = Vector(pos.z, pos.x, pos.y);
				rot = Maths::LHToRHRotation(rot);
				// Pitch, Heading, Roll
				rot = Vector(RadToDeg(-rot.y), RadToDeg(-rot.x), RadToDeg(-rot.z));

				sThread.SendClient(s_guiData->activeClient, "mirv_input position "
					+ String::FloatToString(pos.x) + " " + String::FloatToString(pos.y) + " "
					+ String::FloatToString(pos.z));
				sThread.SendClient(s_guiData->activeClient, "mirv_input angles "
					+ String::FloatToString(rot.x) + " " + String::FloatToString(rot.y) + " "
					+ String::FloatToString(rot.z));
				sThread.SendClient(s_guiData->activeClient, "mirv_input fov real "
					+ String::FloatToString(fov));

				// Timers
				Timer = maxon::TimeValue::GetTime() - Timer;
				SetString(TXT_FTIME, Timer.ToString(nullptr), 0, FLAG_CENTER_HORIZ);
				TimerAverage(Timer);
				SetString(TXT_AVGTIME, ((maxon::TimeValue)TimerAverage).ToString(nullptr), 0, FLAG_CENTER_HORIZ);
			};
		}
		else
		{
			// We are receive, client is transmit
			UpdateCamera = [&]()
			{
				maxon::TimeValue Timer = maxon::TimeValue::GetTime();

				Vector pos = Vector(sThread.GetPositionVec());
				Vector rot = Vector(sThread.GetRotationVec());

				// Left/Right, Up/Down, Forwards/Backwards
				pos = Vector(pos.y, pos.z, pos.x);
				pos = Maths::RHToLHCoordinate(pos);

				// We will also negate the rotations since c4d uses the opposite rotation direction
				// to a typical left-handed system.
				// Heading, Pitch, Bank
				rot = Vector(DegToRad(-rot.z), DegToRad(-rot.y), DegToRad(-rot.x));
				rot = Maths::RHToLHRotation(rot);

				auto m = HPBToMatrix(rot, ROTATIONORDER::HPB);
				m.off = pos;
				m = HPBToMatrix(s_userConfig->orientation, ROTATIONORDER::HPB) * m;
				if (s_userConfig->globalcoords) s_guiData->cameraObj->SetMg(m);
				else s_guiData->cameraObj->SetMl(m);

				s_guiData->cameraObj->SetParameter(CAMERAOBJECT_FOV, DegToRad(sThread.GetFov()), DESCFLAGS_SET::NONE);
				DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION | DRAWFLAGS::NO_EXPRESSIONS); // slow, blocking
				//EventAdd(); // fast, probably just calls drawviews and throws away late events

				// Timers
				Timer = maxon::TimeValue::GetTime() - Timer;
				SetString(TXT_FTIME, Timer.ToString(nullptr), 0, FLAG_CENTER_HORIZ);
				TimerAverage(Timer);
				SetString(TXT_AVGTIME, ((maxon::TimeValue)TimerAverage).ToString(nullptr), 0, FLAG_CENTER_HORIZ);
			};
		}

		StopUpdate();

		if (setup_client)
		{
			auto& sThread = ServerThread::GetInstance();
			sThread.SendClient(s_guiData->activeClient, "echo " + GeLoadString(transmit ? STR_CLIENT_LIVE_RECEIVE : STR_CLIENT_LIVE_TRANSMIT));
			if (transmit) sThread.SendClient(s_guiData->activeClient, "mirv_input camera");
			else sThread.SendClient(s_guiData->activeClient, "mirv_pgl dataStart");
		}


		s_updateCameraTimer = maxon::TimerInterface::AddPeriodicTimer
		(maxon::Seconds(1.0f / (Float)s_userConfig->pollrate), UpdateCamera, maxon::JobQueueInterface::GetMainThreadQueue()).GetValue();
		s_guiData->updating = true;

		static TimeWindowCount<2000> OverloadCatch;
		OverloadCatch.Reset();

		s_updateCameraTimer.ObservableTimerOverload().AddObserver([this](const maxon::TimeValue &duration, const maxon::TimeValue &maxDuration)
			{
				OverloadCatch.Count();

				// if over 15% of the calls in the last 2 seconds have overloaded
				if ((OverloadCatch.GetCounts() / 2) > (s_userConfig->pollrate * 0.15f))
				{
					this->SetStatusText(2000, "OVERLOAD DETECTED");
					OverloadCatch.Reset();
				}

				SetString(TXT_MAXTIME, maxDuration.ToString(nullptr));
			}).UncheckedGetValue();
			return true;
	}

	void Gui::StopUpdate()
	{
		if (s_guiData->updating && s_updateCameraTimer)
		{
			s_updateCameraTimer.CancelAndWait();
			maxon::TimerInterface::Free(s_updateCameraTimer);
			s_updateCameraTimer = nullptr;
		}

		s_guiData->updating = false;
		ResetFunctionMetrics();
	}

	Bool Gui::IsCameraValid()
	{
		if (!s_guiData->cameraObj) return false;
		if (s_guiData->cameraName.IsEmpty()) return false;

		// First search via text search
		BaseObject* o = GetActiveDocument()->SearchObject(s_guiData->cameraName);
		if (o == s_guiData->cameraObj) return true;

		// Exaustively check each object in the document
		std::function<Bool(BaseObject*)> CheckObjects = [&CheckObjects, this](BaseObject* obj)
		{
			while (obj != nullptr)
			{
				if (CheckObjects(obj->GetDown()))
					return true;
				if (obj == s_guiData->cameraObj)
					return true;
				obj = obj->GetNext();
			}

			return false;
		};

		auto doc = GetActiveDocument();
		return CheckObjects(doc->GetFirstObject());
	}

	void Gui::ResetCamera()
	{
		if (s_guiData->updating) StopUpdate();

		SetString(TXT_CAMERANAME, "<< " + GeLoadString(STR_NO_CAMERA) + " >>");
		s_guiData->cameraObj = nullptr;
		s_guiData->cameraName = ""_s;

		// deselect + dataStop client
		if (s_guiData->activeClient != -1)
		{
			BaseContainer data;
			_LVClients.GetItem(s_guiData->activeClient, &data);
			data.SetBool((Int32)LV_CLIENTS_ID::CHCK, false);
			_LVClients.SetItem(s_guiData->activeClient, data);
			ServerThread::GetInstance().SendClient(s_guiData->activeClient, "mirv_pgl dataStop");
			s_guiData->activeClient = -1;
		}
	}

	void Gui::SetUI(Bool enable)
	{
		for (Int32 i = _HLL_UI_GADGETS_BEGIN_; i < _HLL_UI_GADGETS_END_; i++)
			Enable(i, enable);
	}

	void Gui::ResetFunctionMetrics()
	{
		SetString(TXT_FTIME, "---"_s);
		SetString(TXT_AVGTIME, "---"_s);
		SetString(TXT_MAXTIME, "---"_s);
	}

	void Gui::DoUpdateCheck()
	{
		String link;
		Bool res = Tools::CheckForUpdates(&link);
		if (!res) SetStatusText(5000, GeLoadString(STR_VERSION_UPTODATE));
		else if (QuestionDialog(STR_VERSION_OUTDATED)) GeOpenHTML(link);
	}

	Bool Gui::CreateLayout()
	{
		if (s_userConfig->checkforupdates == -1)
		{
			s_userConfig->checkforupdates = QuestionDialog(STR_ASK_AUTO_UPDATE) ? 1 : 0;
			s_userConfig->SaveFile(Globals::userConfigFile);
		}

		if (s_userConfig->checkforupdates == 1)
		{
			// check for update (we only do this once to avoid pestering the user
			// if they open/close the gui multiple times in a session).
			static Bool UpdateChecked = false;
			if (!UpdateChecked)
			{
				DoUpdateCheck();
				UpdateChecked = true;
			}
		}

		Bool res = LoadDialogResource(DLG_HLL, nullptr, 0);
		if (res)
		{
			_LVClients.AttachListView(this, LV_CLIENTS);
			_LVClients.DataChanged();
			this->SetTimer(HLL_STATUSBAR_POLL_RATE);

			// Init banner
			this->_banner = NewObj(Banner).GetValue();
			if (!this->AttachUserArea(*_banner, IMG_BANNER, USERAREAFLAGS::NONE))
				return false;
			this->_banner->LayoutChanged();

			// Create Menu
			MenuFlushAll();

			MenuSubBegin(GeLoadString(STR_UPDATE));
			MenuAddString(CHK_CHECK_FOR_UPDATES, GeLoadString(STR_CHECK_FOR_UPDATES));
			MenuAddSeparator();
			MenuAddString(BTN_CHECK_FOR_UPDATE_NOW, GeLoadString(STR_CHECK_NOW));
			MenuSubEnd();

			MenuSubBegin(GeLoadString(STR_SUPPORT_HLL));
			MenuAddString(BTN_DONATE_PAYPAL, GeLoadString(STR_DONATE_PAYPAL));
			MenuSubEnd();

			MenuFinished();

			BaseContainer layout;
			layout.SetInt32((Int32)LV_CLIENTS_ID::CHCK, LV_COLUMN_CHECKBOX);
			layout.SetInt32((Int32)LV_CLIENTS_ID::NAME, LV_COLUMN_BUTTON);
			layout.SetInt32((Int32)LV_CLIENTS_ID::DISC, LV_COLUMN_BUTTON);

			_LVClients.SetLayout(3, layout);
			_LVClients.DataChanged();

			InitValues();
		}

		return res;
	}

	Bool Gui::InitValues()
	{
		Bool bCameraValid = IsCameraValid();
		if (!bCameraValid) ResetCamera();
		if (!SetString(BTN_LISTEN,
			GeLoadString(s_guiData->listening ? STR_STOP_LISTEN : STR_START_LISTEN)))		return false;	// Listen Button
		if (!SetDegree(VEC_ORIENTATION_X, s_userConfig->orientation.x))						return false;	// Orientation X
		if (!SetDegree(VEC_ORIENTATION_Y, s_userConfig->orientation.y))						return false;	// Orientation Y
		if (!SetDegree(VEC_ORIENTATION_Z, s_userConfig->orientation.z))						return false;	// Orientation Z
		if (!SetString(BTN_MAPPING, s_guiData->mapping))									return false;	// Mapping
		if (!SetString(ETXT_HOSTNAME, s_userConfig->hostname))								return false;	// Hostname
		if (!SetInt32(ETXT_PORT, s_userConfig->port, 1, 65535, 1, 0, 0, 65535))				return false;	// Port
		if (!SetInt32(ETXT_POLLRATE, s_userConfig->pollrate, 1, 360, 1, 0, 1, 360))			return false;	// Pollrate
		if (!MenuInitString(CHK_CHECK_FOR_UPDATES, true, s_userConfig->checkforupdates))	return false;	// Check For Updates

		ResetFunctionMetrics();

		// Handle the case where GUI is reopened while the server is already running
		if (s_guiData->listening)
		{
			BaseContainer data;
			auto Clients = ServerThread::GetInstance().GetClients();

			for (size_t i = 0; i < Clients.size(); i++)
			{
				if (s_guiData->activeClient >= 0 && s_guiData->activeClient == Int32(i))
					data.SetBool((Int32)LV_CLIENTS_ID::CHCK, true);
				else data.SetBool((Int32)LV_CLIENTS_ID::CHCK, false);

				data.SetString((Int32)LV_CLIENTS_ID::NAME, String(Clients[i]._name));
				data.SetString((Int32)LV_CLIENTS_ID::DISC, GeLoadString(STR_DISCONNECT));
				_LVClients.SetItem(Int32(i), data);
			}
			_LVClients.DataChanged();
		}

		return true;
	}

	Bool Gui::Command(Int32 id, const BaseContainer &msg)
	{
		// Called when GUI is interacted with.
		if (id == BTN_CHECK_FOR_UPDATE_NOW)
		{
			DoUpdateCheck();
			return true;
		}

		if (id == BTN_DONATE_PAYPAL)
		{
			GeOpenHTML("https://www.paypal.me/xNWP"_s);
			return true;
		}

		if (id == BTN_LISTEN)
		{
			if (!s_guiData->listening)
			{
				SetUI(false);
				Enable(ETXT_HOSTNAME, false);
				Enable(ETXT_PORT, false);

				String h, p;
				this->GetString(ETXT_HOSTNAME, h);
				this->GetString(ETXT_PORT, p);

				Bool err = false;
				Int32 port = p.ToInt32(&err);

				if (err)
				{
					Tools::LogError(GeLoadString(STR_SERVER_LISTEN_FAILED, h, p));
					SetStatusText(2000, GeLoadString(STR_SERVER_LISTEN_FAILED, h, p));
					return true;
				}

				// This can fail, we will use an event message from the server to signify whether or not
				// the listening succeeded.
				ServerThread::GetInstance().StartServer(h, port);
			}
			else
			{
				SetUI(false);
				StopUpdate();
				if (!ServerThread::GetInstance().CloseServer())
				{
					String msg = "Failed to close listen server.";
					Tools::LogError(msg);
					SetStatusText(2000, msg);
				}
				else
				{
					this->SetString(BTN_LISTEN, GeLoadString(STR_START_LISTEN));
					s_guiData->listening = false;
					String msg = GeLoadString(STR_SERVER_LISTEN_CLOSED);
					Tools::Log(msg);
					ClearStatusText();
					SetStatusText(2000, msg);
				}

				SetUI(true);
				if (!s_guiData->listening)
				{
					Enable(ETXT_HOSTNAME, true);
					Enable(ETXT_PORT, true);
				}
			}

			return true;
		}

		if (id == ETXT_POLLRATE)
		{
			GetInt32(ETXT_POLLRATE, s_userConfig->pollrate);
			if (s_guiData->updating) StartUpdate(false);
			return true;
		}

		if (id == BTN_SETCAMERA)
		{
			if (s_guiData->updating) StopUpdate();
			BaseObject *o = GetActiveDocument()->GetActiveObject();
			if (!o)
			{
				SetStatusText(2000, GeLoadString(STR_CAM_NO_SELECT));
				return true;
			}

			if (o->GetType() != Ocamera)
			{
				String m = GeLoadString(STR_NOT_A_CAMERA, o->GetName());
				SetStatusText(2000, m);
				return true;
			}

			if (!this->SetString(TXT_CAMERANAME, "<< " + o->GetName() + " >>"))
				return false;

			s_guiData->cameraName = o->GetName();
			s_guiData->cameraObj = o;
			if (s_guiData->updating) StartUpdate(false);
			return true;
		}

		if (id == BTN_COPY_URI)
		{
			String h, p;
			this->GetString(ETXT_HOSTNAME, h);
			this->GetString(ETXT_PORT, p);
			CopyToClipboard("mirv_pgl url \"ws://" + h + ":" + p + "\";mirv_pgl start");
		}

		if (id == BTN_MAPPING)
		{
			if (s_guiData->listening)
			{
				// Deselect all clients on a mapping switch
				auto& sThread = ServerThread::GetInstance();
				for (Int32 i = 0; i < _LVClients.GetItemCount(); i++)
				{
					BaseContainer data;
					_LVClients.GetItem(i, &data);
					data.SetBool((Int32)LV_CLIENTS_ID::CHCK, false);
					_LVClients.SetItem(i, data);
				}
				_LVClients.DataChanged();

				// Stop any and all camera updating
				StopUpdate();
				if (s_guiData->activeClient != -1)
				{
					sThread.SendClient(s_guiData->activeClient, "mirv_input end");
					sThread.SendClient(s_guiData->activeClient, "echo " + GeLoadString(STR_CLIENT_INACTIVE));
					sThread.SendClient(s_guiData->activeClient, "mirv_pgl dataStop");
				}
				s_guiData->activeClient = -1;
			}

			if (s_guiData->mapping == GeLoadString(STR_TARGET_CINEMA4D)) s_guiData->mapping = GeLoadString(STR_TARGET_CSGO);
			else s_guiData->mapping = GeLoadString(STR_TARGET_CINEMA4D);
			SetString(BTN_MAPPING, s_guiData->mapping);

			return true;
		}

		if (id == LV_CLIENTS)
		{
			Int32 action, item, col;
			BaseContainer colData;
			action = msg.GetInt32(BFM_ACTION_VALUE);
			auto& sThread = ServerThread::GetInstance();

			if (action == LV_SIMPLE_CHECKBOXCHANGED)
			{
				Int32 sid = msg.GetInt32(LV_SIMPLE_ITEM_ID);
				if (!IsCameraValid())
				{
					SetStatusText(4000, GeLoadString(STR_CAM_NO_SELECT));
					// Deselect the client
					BaseContainer data;
					_LVClients.GetItem(sid, &data);
					data.SetBool((Int32)LV_CLIENTS_ID::CHCK, false);
					_LVClients.SetItem(sid, data);
					_LVClients.DataChanged();

					return true;
				}

				// stop sending data to active client (if they exist)
				if (s_guiData->activeClient != -1)
				{
					StopUpdate();
					sThread.SendClient(s_guiData->activeClient, "echo " + GeLoadString(STR_CLIENT_INACTIVE));
					sThread.SendClient(s_guiData->activeClient, "mirv_pgl dataStop");
					sThread.SendClient(s_guiData->activeClient, "mirv_input end");
				}

				// Check if this is a deselect
				if (sid == s_guiData->activeClient)
				{
					s_guiData->activeClient = -1;
					return true;
				}

				// Set ActiveClient
				s_guiData->activeClient = sid;

				// Deselect Other Clients
				for (Int32 i = 0; i < _LVClients.GetItemCount(); i++)
				{
					if (i == s_guiData->activeClient)
						continue;

					BaseContainer data;
					Int32 id;

					_LVClients.GetItemLine(i, &id, &data);
					data.SetBool((Int32)LV_CLIENTS_ID::CHCK, false);
					_LVClients.SetItem(id, data);
				}

				_LVClients.DataChanged();

				this->SetStatusText(2000, GeLoadString(STR_ACTIVE_CLIENT, (String)ServerThread::GetInstance().GetClients()[s_guiData->activeClient]._name));

				if (!StartUpdate()) Tools::LogError("Failed to start CameraUpdate function.");
				return true;
			}


			{
				item = msg.GetInt32(LV_SIMPLE_ITEM_ID);
				col = msg.GetInt32(LV_SIMPLE_COL_ID);

				if (action == LV_SIMPLE_BUTTONCLICK && col == (Int32)LV_CLIENTS_ID::DISC)
				{
					// disconnect
					sThread.DisconnectClient(item);
					return true;
				}

				// This is a disgusting workaround so that the middle (name in this case)
				// button will actually be picked up, for some reason it's not sending the
				// LV_SIMPLE_BUTTONCLICK event for the middle button as of R21 (need to verify that this is a bug still).
				// TODO: FIX THIS, this code is buggy and slow, need to figure out the source of the bug!!!

				if (col == (Int32)LV_CLIENTS_ID::NAME && (action & (LV_SIMPLE_FOCUSITEM | LV_SIMPLE_FOCUSITEM_NC)))
				{
					using namespace std::chrono_literals;
					std::this_thread::sleep_for(250ms); // this delay gives the button time to send the event (for whatever reason)
				}

				// rename
				if (col == (Int32)LV_CLIENTS_ID::NAME && action == LV_SIMPLE_BUTTONCLICK)
				{
					if (!_LVClients.GetItem(item, &colData))
					{
						Tools::LogError(GeLoadString(STR_UNEXPECTED_ERROR, "hll_gui.cpp LV_SIMPLE_BUTTONCLICK"_s));
						return false;
					}

					String str = colData.GetString((Int32)LV_CLIENTS_ID::NAME);
					if (RenameDialog(&str))
					{
						colData.SetString((Int32)LV_CLIENTS_ID::NAME, str);
						_LVClients.SetItem(item, colData);
						_LVClients.DataChanged();
					}

					sThread.RenameClient(item, str.GetCStringCopy());

					return true;
				}
			}
		}

		if (id == VEC_ORIENTATION_X || id == VEC_ORIENTATION_Y || id == VEC_ORIENTATION_Z)
		{
			GetFloat(VEC_ORIENTATION_X, s_userConfig->orientation.x);
			GetFloat(VEC_ORIENTATION_Y, s_userConfig->orientation.y);
			GetFloat(VEC_ORIENTATION_Z, s_userConfig->orientation.z);
			return true;
		}

		if (id == CHK_CHECK_FOR_UPDATES)
		{
			s_userConfig->checkforupdates = !s_userConfig->checkforupdates;
			MenuInitString(CHK_CHECK_FOR_UPDATES, true, s_userConfig->checkforupdates);
			return true;
		}

		return GeDialog::Command(id, msg);
	}

	Bool Gui::CoreMessage(Int32 id, const BaseContainer &msg)
	{
		// Catch scene change
		//if (id == EVMSG_CHANGE) InitValues();

		// Catch messages from server to us.
		if (id == Globals::pluginId)
		{
			Int32 message = (Int32)reinterpret_cast<intptr_t>(msg.GetVoid(BFM_CORE_PAR1));
			Int32 param = (Int32)reinterpret_cast<intptr_t>(msg.GetVoid(BFM_CORE_PAR2));

			switch (message)
			{
			case HLL_EVMSG_CLIENT_CONNECT:
			{
				BaseContainer data;
				data.SetBool((Int32)LV_CLIENTS_ID::CHCK, false);
				data.SetString((Int32)LV_CLIENTS_ID::NAME, ServerThread::GetInstance().GetClients()[param]._name);
				data.SetString((Int32)LV_CLIENTS_ID::DISC, GeLoadString(STR_DISCONNECT));
				_LVClients.SetItem(param, data);
				_LVClients.DataChanged();

				SetStatusText(5000, GeLoadString(STR_SERVER_CONNECT, data.GetString((Int32)LV_CLIENTS_ID::NAME)));
				break;
			}

			case HLL_EVMSG_CLIENT_DISCONNECT:
			{
				// Stop updating if disconnect is active client
				if (param == s_guiData->activeClient) StopUpdate();
				Int32 ClientsSize = ServerThread::GetInstance().GetClients().size();

				//Bool bReorder = !(param == (ClientsSize - 1));
				BaseContainer dat;
				_LVClients.GetItem(param, &dat);
				String cname = dat.GetString((Int32)LV_CLIENTS_ID::NAME);
				_LVClients.RemoveItem(param);

				SetStatusText(5000, GeLoadString(STR_SERVER_DISCONNECT, cname));

				/*
				if (bReorder)
				{
					BaseContainer data;
					for (Int32 i = param; i < ClientsSize; i++)
					{
						_LVClients.GetItem(i + 1, &data);
						_LVClients.SetItem(i, data);
						_LVClients.RemoveItem(i + 1);
						data = BaseContainer();
					}
				}
				*/

				_LVClients.DataChanged();

				if (s_guiData->activeClient == param)
					s_guiData->activeClient = -1;
				break;
			}

			case HLL_EVMSG_LISTEN_SUCCESS:
			{
				SetUI(true);
				String h, p;
				this->GetString(ETXT_HOSTNAME, h);
				this->GetString(ETXT_PORT, p);
				this->SetString(BTN_LISTEN, GeLoadString(STR_STOP_LISTEN));
				this->Enable(BTN_LISTEN, true);

				this->SetStatusText(0, GeLoadString(STR_SERVER_LISTEN_SUCCESS, h, p));
				Tools::Log(GeLoadString(STR_SERVER_LISTEN_SUCCESS, h, p));
				s_guiData->listening = true;
				break;
			}

			case HLL_EVMSG_LISTEN_FAILED:
			{
				SetUI(true);
				String h, p;
				this->GetString(ETXT_HOSTNAME, h);
				this->GetString(ETXT_PORT, p);
				s_guiData->listening = false;
				SetStatusText(5000, GeLoadString(STR_SERVER_LISTEN_FAILED, h, p));
				Tools::LogError(GeLoadString(STR_SERVER_LISTEN_FAILED, h, p));
				break;
			}

			case HLL_EVMSG_DATASTART:
			{
				String s;
				this->GetString(BTN_MAPPING, s);
				if (param != s_guiData->activeClient || s == GeLoadString(STR_TARGET_CSGO))
				{
					// Don't update if this is not the client we want or we want to be transmitting data
					BaseContainer dat;
					_LVClients.GetItem(param, &dat);
					String cname = dat.GetString((Int32)LV_CLIENTS_ID::NAME);
					Tools::Log(GeLoadString(STR_SERVER_BAD_DATASTART, cname));
					SetStatusText(5000, GeLoadString(STR_SERVER_BAD_DATASTART, cname));

					// Ask them politely (yet firmly) to stop sending data
					auto& sThread = ServerThread::GetInstance();
					sThread.SendClient(param, "mirv_pgl dataStop");
					sThread.SendClient(param, "echo " + GeLoadString(STR_CLIENT_BAD_DATASTART));
				}
				break;
			}

			case HLL_EVMSG_DATASTOP:
			{
				// only cancel if the client that issued dataStop was the active client
				if (param == s_guiData->activeClient)
				{
					StopUpdate();

					// check if the client issued this dataStop or if it was us, react accordingly.
					BaseContainer dat;
					_LVClients.GetItem(param, &dat);

					// it wasn't us
					if (dat.GetBool((Int32)LV_CLIENTS_ID::CHCK))
					{
						String cname = dat.GetString((Int32)LV_CLIENTS_ID::NAME);
						Tools::Log(GeLoadString(STR_SERVER_BAD_DATASTOP, cname));
						SetStatusText(5000, GeLoadString(STR_SERVER_BAD_DATASTOP, cname));
						// deselect their entry
						dat.SetBool((Int32)LV_CLIENTS_ID::CHCK, false);
						_LVClients.SetItem(param, dat);
						_LVClients.DataChanged();
						_LVClients.Redraw();
						s_guiData->activeClient = -1;
					}
				}
				break;
			}

			default:
			{
				Tools::LogError("Unknown event received by gui");
				break;
			}
			}

			ServerThread::GetInstance().EventHandled();
			return true;
		}

		return GeDialog::CoreMessage(id, msg);
	}

	void Gui::DestroyWindow()
	{
		SetTimer(0);
	}

	void Gui::Timer(const BaseContainer &msg)
	{
		// set status to message at the top of the queue (LIFO)
		StatusMessage *a = s_StatusMessageQueue.GetLast();
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
		for (Int32 i = 0; i < s_StatusMessageQueue.GetCount(); i++)
		{
			// ignore 'sticky' messages
			if (s_StatusMessageQueue[i].timeleft == 0)
				continue;

			Int32 tl = s_StatusMessageQueue[i].timeleft - HLL_STATUSBAR_POLL_RATE;
			if (tl <= 0)
				s_StatusMessageQueue.Erase(i--, 1).GetValue();
			else
				s_StatusMessageQueue[i].timeleft = tl;

			bActive = true;
		}

		// stop timer if only sticky messages
		if (!bActive)
		{
			this->SetTimer(0);
			return;
		}
	}

	void Gui::SetStatusText(Int32 duration, const String &msg)
	{
		StatusMessage a(duration, msg);
		s_StatusMessageQueue.Append(a).GetValue();
		this->SetTimer(HLL_STATUSBAR_POLL_RATE);
	}

	void Gui::ClearStatusText()
	{
		s_StatusMessageQueue.Reset();
	}

	Bool GuiCommand::Execute(BaseDocument *doc, GeDialog* parentManager)
	{
		// Called when plugin is selected from menu.
		oGui = NewObjClear(Gui);
		if (!oGui->Open(DLG_TYPE::ASYNC, Globals::pluginId))
			return false;

		return true;
	}

	Gui::Banner::~Banner()
	{
		BaseBitmap::Free(bmp);
	}

	void Gui::Banner::DrawMsg(Int32 x1, Int32 y1, Int32 x2, Int32 y2, const BaseContainer &msg)
	{
		bmp = BaseBitmap::Alloc();
		Filename imagePath(GeGetPluginPath() + "\\res" + "\\banner.png");
		bmp->Init(imagePath);

		DrawBitmap(bmp, 0, 0, bmp->GetBw(), bmp->GetBh(), 0, 0, bmp->GetBw(), bmp->GetBh(), BMP_ALLOWALPHA);

#pragma push_macro("DrawText")
#undef DrawText
		DrawSetTextCol(Vector(1.0f), COLOR_TRANS);
		DrawSetFont(FONT_MONOSPACED);
		DrawText(Globals::Version::fullString(), HLL_BANNER_WIDTH - 7, HLL_BANNER_HEIGHT - 9, DRAWTEXT_HALIGN_RIGHT | DRAWTEXT_VALIGN_BOTTOM);
#pragma pop_macro("DrawText")
	}

	Bool RegisterHLL(std::shared_ptr<Tools::UserConfig> userConfig)
	{
		s_userConfig = userConfig;
		s_guiData = std::make_unique<GuiData>();

		BaseBitmap *icon = BaseBitmap::Alloc();
		Filename icoPath(GeGetPluginPath() + "\\res" + "\\icon.png");
		icon->Init(icoPath);
		GuiCommand *oGuiCommand = NewObjClear(GuiCommand);
		return RegisterCommandPlugin(Globals::pluginId, "HLAELiveLink"_s, 0,
			icon, GeLoadString(STR_HELP_STRING), oGuiCommand);
	}
}