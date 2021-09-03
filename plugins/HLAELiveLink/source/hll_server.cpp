// Copyright (c) 2018 Brett Anthony. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "hll_server.h"
#include "../res/c4d_symbols.h"
#include "hll_globals.h"
#include <sstream>
#include "maxon/thread.h"
#pragma warning(push, 0)
#ifdef MAXON_TARGET_WINDOWS
#include <algorithm>
#include "App.h"
#include "libusockets.h"
#include "maxon/utilities/undef_win_macros.h"
#endif
#pragma warning(pop)
#include "maxon/conditionvariable.h"

#include "hll_tools.h"

static maxon::ThreadRefTemplate<HLL::ServerThread> s_ServerThread;

HLL::ServerThread& HLL::ServerThread::GetInstance()
{
	static std::mutex mut;
	std::scoped_lock<std::mutex> lk(mut);
	if (!s_ServerThread)
	{
		s_ServerThread = ServerThread::Create().GetValue();
		s_ServerThread->_guiHandlingMessage = maxon::ConditionVariableRef::Create().GetValue();
		s_ServerThread->_guiHandlingMessage.Set();
	}
	return *s_ServerThread;
}

maxon::Result<void> HLL::ServerThread::operator()()
{
	uWS::App::WebSocketBehavior<ClientData> wsBehaviour;
	wsBehaviour.compression = uWS::CompressOptions::DISABLED;
	wsBehaviour.maxPayloadLength = 256; // 256 Bytes
	wsBehaviour.maxBackpressure = 4 * 1024; // 4 KiB
	wsBehaviour.closeOnBackpressureLimit = false;
	wsBehaviour.sendPingsAutomatically = true;
	wsBehaviour.idleTimeout = 10;
	wsBehaviour.upgrade = nullptr;

	wsBehaviour.open = [this](auto* ws)
	{
		_guiHandlingMessage.Wait(); // don't modify clients until gui is done handling event
		static Int index = -1;
		String ClientName = GeLoadString(STR_CLIENT) + " " + String::IntToString(++index);
		HLL::Tools::Log(GeLoadString(STR_SERVER_CONNECT, ClientName));
		String HelloClient = "echo " + GeLoadString(STR_CLIENT_CONNECT, ClientName);

		SendSocket(HelloClient, ws);
		_clients.push_back(Client(ClientName.GetCStringCopy(), ws));

		// Let Gui know we have a new client + their index in the list.
		_guiHandlingMessage.Clear();
		SpecialEventAdd(Globals::pluginId, HLL_EVMSG_CLIENT_CONNECT, _clients.size() - 1);
	};

	wsBehaviour.close = [this](auto* ws, int, std::string_view)
	{
		_guiHandlingMessage.Wait();

		// Find the client who disconnected
		for (size_t i = 0; i < _clients.size(); i++)
		{
			if (_clients[i]._socket == ws)
			{
				// erase the entry from our clients vector
				String ClientName = _clients[i]._name;
				_clients.erase(_clients.begin() + i);
				HLL::Tools::Log(GeLoadString(STR_SERVER_DISCONNECT, ClientName));

				// Let Gui know we lost a client
				_guiHandlingMessage.Clear();
				SpecialEventAdd(Globals::pluginId, HLL_EVMSG_CLIENT_DISCONNECT, i);
				return; // exit
			}
		}

		// should never reach this point !!!
		HLL::Tools::LogError(GeLoadString(STR_UNEXPECTED_ERROR, "hll_server.cpp ClientCloseCallback"_s));
	};

	wsBehaviour.message = [this](auto* ws, std::string_view message, uWS::OpCode)
	{
		// Keep track of where we are in the data
		UInt64 it = 0;

		// Catch empty buffer
		if (message.length() == 0)
			return;

		while (it < message.length())
		{
			String cmd = "";
			while ((message[it] != '\0') && (it < message.length()))
			{
				cmd.AppendChar(message[it]).GetValue();
				it++;
			}
			it++;

			if (cmd == "dataStart")
			{
				// Find the client who is sending data
				for (size_t i = 0; i < _clients.size(); i++)
				{
					if (_clients[i]._socket == ws)
					{
						// Notify GUI
						_guiHandlingMessage.Clear();
						SpecialEventAdd(Globals::pluginId, HLL_EVMSG_DATASTART, i);
						return; // exit
					}
				}
				continue;
			}

			if (cmd == "dataStop")
			{
				// Find the client who stopped sending data
				for (size_t i = 0; i < _clients.size(); i++)
				{
					if (_clients[i]._socket == ws)
					{
						// Notify GUI
						_guiHandlingMessage.Clear();
						SpecialEventAdd(Globals::pluginId, HLL_EVMSG_DATASTOP, i);
						return; // exit
					}
				}
				continue;
			}

			if (cmd == "cam")
			{
				// skip time
				it += 4;

				std::scoped_lock<std::mutex> lock(_dataMutex);
				this->_xp = FourByteFloatLE(message, it);
				it += 4;
				this->_yp = FourByteFloatLE(message, it);
				it += 4;
				this->_zp = FourByteFloatLE(message, it);
				it += 4;
				this->_xr = FourByteFloatLE(message, it);
				it += 4;
				this->_yr = FourByteFloatLE(message, it);
				it += 4;
				this->_zr = FourByteFloatLE(message, it);
				it += 4;
				this->_fov = FourByteFloatLE(message, it);
				it += 4;

				continue;
			}
		}
	};

	uWS::App app = uWS::App().ws<ClientData>("/*", std::move(wsBehaviour));
	app.listen(_port, [this](auto* listen_socket)
		{
			if (listen_socket)
			{
				String m = GeLoadString(STR_STARTING_SERVER, String(_host), String::IntToString(_port));
				HLL::Tools::Log(m);

				// Notify GUI of successful listen
				_guiHandlingMessage.Clear();
				SpecialEventAdd(Globals::pluginId, HLL_EVMSG_LISTEN_SUCCESS);
				this->_listen_socket = listen_socket;
			}
			else
			{
				// server failed to run, notify gui.
				_guiHandlingMessage.Clear();
				SpecialEventAdd(Globals::pluginId, HLL_EVMSG_LISTEN_FAILED);
			}
		});
	
	app.run(); // blocking if successful
	return maxon::OK;
}

void HLL::ServerThread::SendClient(Int32 index, const String &command)
{
	SendSocket(command, _clients[index]._socket);
}

void HLL::ServerThread::SendSocket(const String& command, void* ws)
{
	std::string msg = "exec " + std::string((command.GetCStringCopy())) + "0";
	msg[4] = msg[msg.length() - 1] = '\0';
	((uWS::WebSocket<false, true, HLL::ClientData>*)ws)->send(msg, uWS::OpCode::BINARY);
}

std::vector<HLL::Client> HLL::ServerThread::GetClients()
{
	return this->_clients;
}

void HLL::ServerThread::RenameClient(Int32 index, const String& str)
{
	_guiHandlingMessage.Wait();
	String oldName = _clients[index]._name;
	_clients[index]._name = str;

	String ntfy = String(oldName) + " => " + str;
	HLL::Tools::Log(GeLoadString(STR_SERVER_RENAME, ntfy));
	SendClient(index, "echo " + GeLoadString(STR_CLIENT_RENAME, ntfy));
}

void HLL::ServerThread::DisconnectClient(Int32 index)
{
	String ClientName = _clients[index]._name;

	SendClient(index, "echo " + GeLoadString(STR_CLIENT_DISCONNECT));
	SendClient(index, "mirv_pgl dataStop; mirv_pgl stop");
}

void HLL::ServerThread::StartServer(const String& host, Int32 port)
{
	if (IsRunning())
	{
		CloseServer();
		Wait();
	}

	_host = host;
	_port = port;

	maxon::Error err = Start().GetError();
	if (err) Tools::LogError(err.GetMessage());
}

Bool HLL::ServerThread::CloseServer()
{
	if (IsRunning())
	{
		for (Int32 i = 0; i < static_cast<Int32>(_clients.size()); i++)
			DisconnectClient(i);

		us_listen_socket_close(false, this->_listen_socket);
	}

	while (IsRunning()) Wait(maxon::Seconds(1));
	s_ServerThread.CancelAndWait();
	s_ServerThread = nullptr;

	return true;
}

Vector32 HLL::ServerThread::GetPositionVec()
{
	std::scoped_lock<std::mutex> lock(_dataMutex);
	return Vector32(this->_xp, this->_yp, this->_zp);
}

Vector32 HLL::ServerThread::GetRotationVec()
{
	std::scoped_lock<std::mutex> lock(_dataMutex);
	return Vector32(this->_xr, this->_yr, this->_zr);
}

Float32 HLL::ServerThread::GetFov()
{
	std::scoped_lock<std::mutex> lock(_dataMutex);
	return this->_fov;
}

void HLL::ServerThread::EventHandled()
{
	_guiHandlingMessage.Set();
}

Float32 HLL::ServerThread::FourByteFloatLE(std::string_view data, UInt64 offset)
{
	float f;
	char s[] = { data[offset], data[offset + 1],
		data[offset + 2], data[offset + 3] };
	memcpy(&f, &s, 4);

	return Float32(f);
}