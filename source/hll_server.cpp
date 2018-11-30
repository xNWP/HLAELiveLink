// Copyright (c) 2018 Brett Anthony. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "hll_server.h"

HLL::ServerThread::ServerThread(const char *hostname, const Int32 &port) : _host(_strdup(hostname)), _port(port),
_xp(0), _yp(0), _zp(0), _xr(0), _yr(0), _zr(0), _fov(0)
{
	this->_hub.onMessage([this](uWS::WebSocket<uWS::SERVER> *ws, char *message, size_t length, uWS::OpCode opCode)
	{
		// Keep track of where we are in the data
		UInt64 it = 0;

		// Catch empty buffer
		if (length == 0)
			return;

		while (it < length)
		{
			String cmd = "";
			while ((*(message + it) != '\0') && (it < length))
			{
				cmd.AppendChar(*(message + it));
				it++;
			}
			it++;

			if (cmd == "dataStart")
			{
				// Find the client who is sending data
				for (size_t i = 0; i < this->_clients.size(); i++)
				{
					if (this->_clients[i]._socket == ws)
					{
						// Notify GUI
						SpecialEventAdd(ID_HLAELIVELINK, HLL_EVMSG_DATASTART, i);
						return; // exit
					}
				}
				continue;
			}

			if (cmd == "dataStop")
			{
				// Find the client who stopped sending data
				for (size_t i = 0; i < this->_clients.size(); i++)
				{
					if (this->_clients[i]._socket == ws)
					{
						// Notify GUI
						SpecialEventAdd(ID_HLAELIVELINK, HLL_EVMSG_DATASTOP, i);
						return; // exit
					}
				}
				continue;
			}

			if (cmd == "cam")
			{
				// skip time
				it += 4;

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
	});

	this->_hub.onConnection([this](uWS::WebSocket<uWS::SERVER> *ws, uWS::HttpRequest req)
	{
		static Int index = -1;

		String ClientName = GeLoadString(STR_CLIENT) + " " + String::IntToString(++index);

		ApplicationOutput("| HLAELiveLink - " + GeLoadString(STR_CLIENT_CONNECT) + "[" + ClientName + "]");

		String HelloClient = "echo " + GeLoadString(STR_HELLO_CLIENT) + " [" + ClientName + "]";

		SendSocket(HelloClient, ws);
		this->_clients.push_back(Client(ClientName.GetCStringCopy(), ws));

		// Let Gui know we have a new client + their index in the list.
		SpecialEventAdd(ID_HLAELIVELINK, HLL_EVMSG_CLIENT_CONNECT, this->_clients.size() - 1);
	});

	this->_hub.onDisconnection([this](uWS::WebSocket<uWS::SERVER> *ws, int code, char *message, size_t length)
	{
		// Find the client who disconnected
		for (size_t i = 0; i < this->_clients.size(); i++)
		{
			if (this->_clients[i]._socket == ws)
			{
				// erase the entry from our clients vector
				String ClientName = this->_clients[i]._name;
				this->_clients.erase(this->_clients.begin() + i);
				ApplicationOutput("| HLAELiveLink - " + GeLoadString(STR_CLIENT_DISCONNECT) + "[" + ClientName + "]");

				// Let Gui know we lost a client
				SpecialEventAdd(ID_HLAELIVELINK, HLL_EVMSG_CLIENT_DISCONNECT, i);
				return; // exit
			}
		}

		// should never reach this point !!!
		// possibility of a race condition, although that should be mitigated by the fact uWS is single threaded.
		ApplicationOutput(GeLoadString(STR_UNEXPECTED_ERROR) + "hll_server.cpp LN 47");
	});
}

maxon::Result<void> HLL::ServerThread::operator ()()
{
	if (this->_hub.listen(this->_host, this->_port))
	{
		String m = GeLoadString(STR_STARTING_SERVER) + " [" + GeLoadString(STR_HOST) +
			": " + this->_host + " " + GeLoadString(STR_PORT) + ": " + String::IntToString(this->_port) + "]";
		ApplicationOutput(m);

		this->_listening = true;

		// Notify GUI of successful listen
		SpecialEventAdd(ID_HLAELIVELINK, HLL_EVMSG_LISTEN_SUCCESS);

		// Start Server
		this->_hub.run();
	}
	else
	{
		// server failed to run, notify gui.
		SpecialEventAdd(ID_HLAELIVELINK, HLL_EVMSG_LISTEN_FAILED);
		return maxon::UnknownError(MAXON_SOURCE_LOCATION);
	}

	return maxon::OK;
}

void HLL::ServerThread::SendClient(Int32 index, const String &command)
{
	SendSocket(command, this->_clients[index]._socket);
}

void HLL::ServerThread::SendSocket(const String &command, uWS::WebSocket<uWS::SERVER> *ws)
{
	String pl = "exec " + command;

	char *cpl = NewMemClear(char, pl.GetCStringLen() + 1).GetValue();
	pl.GetCString(cpl, pl.GetCStringLen() + 1);
	// Manually inserting null-terminator since C4D's GetCString function ignores it otherwise.
	cpl[4] = '\0';

	ws->send(cpl, pl.GetCStringLen() + 1, uWS::OpCode::TEXT);
}

const std::vector<HLL::Client>& HLL::ServerThread::GetClients() const
{
	return this->_clients;
}

void HLL::ServerThread::RenameClient(const int &index, const char *str)
{
	char *oldName = this->_clients[index]._name;
	this->_clients[index]._name = _strdup(str);

	String ntfy = GeLoadString(STR_SERVER_RENAME) + oldName + " => " + str;
	ApplicationOutput(ntfy);
	SendClient(index, "echo " + ntfy);
}

void HLL::ServerThread::DisconnectClient(const int &index)
{
	String ClientName = this->_clients[index]._name;

	SendClient(index, "echo " + GeLoadString(STR_SERVER_DISCONNECT));
	SendClient(index, "mirv_pgl dataStop; mirv_pgl stop");
	ApplicationOutput(GeLoadString(STR_SERVER_DISCONNECT_SERVERSIDE) + "[" + ClientName + "]");
}

void HLL::ServerThread::CloseServer()
{
	for (Int32 i = 0; i < static_cast<Int32>(this->_clients.size()); i++)
		DisconnectClient(i);

	this->_hub.getDefaultGroup<uWS::SERVER>().close();

	this->_listening = false;
}

Bool HLL::ServerThread::Closed()
{
	return !this->_listening;
}

Vector32 HLL::ServerThread::GetPositionVec()
{
	return Vector32(this->_xp, this->_yp, this->_zp);
}

Vector32 HLL::ServerThread::GetRotationVec()
{
	return Vector32(this->_xr, this->_yr, this->_zr);
}

Float32 HLL::ServerThread::GetFov()
{
	return this->_fov;
}

Float32 HLL::ServerThread::FourByteFloatLE(char * data, UInt64 offset)
{
	float f;
	char s[] = { data[offset], data[offset + 1],
		data[offset + 2], data[offset + 3] };
	memcpy(&f, &s, 4);

	return Float32(f);
}