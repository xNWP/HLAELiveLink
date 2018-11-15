// Copyright (c) 2018 Brett Anthony. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "hll_server.h"

HLL::ServerThread::ServerThread()
{
	this->_hub.onMessage([](uWS::WebSocket<uWS::SERVER> *ws, char *message, size_t length, uWS::OpCode opCode)
	{
		// to-do: interpret and store data
	});

	this->_hub.onConnection([this](uWS::WebSocket<uWS::SERVER> *ws, uWS::HttpRequest req)
	{
		static Int index = -1;

		String ClientName = GeLoadString(STR_CLIENT) + " " + String::IntToString(++index);

		ApplicationOutput(GeLoadString(STR_CLIENT_CONNECT) + "[" + ClientName + "]");

		String HelloClient = "echo " + GeLoadString(STR_HELLO_CLIENT) + " [" + ClientName + "]";

		SendSocket(HelloClient, ws);
		this->_clients.push_back(Client(ClientName.GetCStringCopy(), ws));

		// Let Gui know we have a new client + their index in the list.
		SpecialEventAdd(ID_HLAELIVELINK, HLL_EVMSG_CLIENT_CONNECT, _clients.size() - 1);
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
				ApplicationOutput(GeLoadString(STR_CLIENT_DISCONNECT) + "[" + ClientName + "]");

				// Let Gui know we lost a client
				SpecialEventAdd(ID_HLAELIVELINK, HLL_EVMSG_CLIENT_DISCONNECT, i);
				return; // exit
			}
		}

		// should never reach this point !!!
		// possibility of a race condition, although that may be mitigated by the fact uWS is single threaded.
		ApplicationOutput(GeLoadString(STR_UNEXPECTED_ERROR) + "hll_server.cpp LN 47");
	});
}

maxon::Result<void> HLL::ServerThread::operator ()()
{
	if (this->_hub.listen("192.168.1.3", 31337))
	{
		String m = GeLoadString(STR_STARTING_SERVER) + " [" + GeLoadString(STR_HOST) +
			": " + "192.168.1.3" + " " + GeLoadString(STR_PORT) + ": 31337" + "]";
		ApplicationOutput(m);

		// Start Server.
		this->_hub.run();
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

void HLL::ServerThread::StopListening()
{
	// Gracefully disconnect each client
	for (Int32 i = 0; i < static_cast<Int32>(this->_clients.size()); i++)
		DisconnectClient(i);

	// Stop the listen server
	this->_hub.getDefaultGroup<uWS::SERVER>().close(1000, "test", 4);
}