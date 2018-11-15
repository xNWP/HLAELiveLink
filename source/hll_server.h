// Copyright (c) 2018 Brett Anthony. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#ifndef HLL_SERVER_H__
#define HLL_SERVER_H__

#include "c4d.h"
#include "../res/c4d_symbols.h"
#include "hll_globals.h"
#include <sstream>
#include "maxon/thread.h"
#include "../uWebSockets/src/uWS.h"

#define HLL_EVMSG_CLIENT_CONNECT		1
#define HLL_EVMSG_CLIENT_DISCONNECT		2

namespace HLL
{
	//------------------------------------------------------//
	/// Provides information about the client as well as their socket.
	//------------------------------------------------------//
	struct Client
	{
		Client(char *name, uWS::WebSocket<uWS::SERVER> *socket) : _name(name), _socket(socket) {}
		char *_name;
		uWS::WebSocket<uWS::SERVER> *_socket;
	};

	//------------------------------------------------------//
	/// Provides the interface for our server to run off the main thread.
	//------------------------------------------------------//
	class ServerThread : public maxon::ThreadInterfaceTemplate<ServerThread>
	{
	public:
		ServerThread();

		//------------------------------------------------------//
		/// The actual worker for the server.
		//------------------------------------------------------//
		maxon::Result<void> operator ()();

		//------------------------------------------------------//
		/// Send a command to a client by index (ServerThread owns the clients).
		/// @param[in] index			The client by index to send the command.
		/// @param[in] command			The command to send.
		//------------------------------------------------------//
		void SendClient(Int32 index, const String &command);

		//------------------------------------------------------//
		/// Send a command over the socket.
		/// @param[in] command			The command to send.
		/// @param[in] ws				The socket to send the command to.
		//------------------------------------------------------//
		void SendSocket(const String &command, uWS::WebSocket<uWS::SERVER> *ws);

		//------------------------------------------------------//
		/// Returns a reference to the clients.
		/// @return std::vector<Client>	The clients.
		//------------------------------------------------------//
		const std::vector<Client>& GetClients() const;

		//------------------------------------------------------//
		/// Rename a client.
		/// @param[in] index			The index of the client.
		/// @param[in] str				The new name of the client.
		//------------------------------------------------------//
		void RenameClient(const int &index, const char *str);

		//------------------------------------------------------//
		/// Gracefully disconnect a client.
		/// @param[in] index			The index of the client.
		//------------------------------------------------------//
		void DisconnectClient(const int &index);
		
		//------------------------------------------------------//
		/// Gracefully disconnects each client and stops the server.
		//------------------------------------------------------//
		void StopListening();

	private:
		std::vector<Client> _clients;
		uWS::Hub _hub;
	};
}

#endif // !HLL_SERVER_H__
