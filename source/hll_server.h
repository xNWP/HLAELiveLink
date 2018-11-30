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
#pragma warning(push, 0)
#include "../uWebSockets/src/uWS.h"
#pragma warning(pop)

#define HLL_EVMSG_CLIENT_CONNECT			1
#define HLL_EVMSG_CLIENT_DISCONNECT			2
#define HLL_EVMSG_LISTEN_SUCCESS			3
#define HLL_EVMSG_LISTEN_FAILED				4
#define HLL_EVMSG_DATASTART					5
#define HLL_EVMSG_DATASTOP					6

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
		ServerThread(const char *hostname, const Int32 &port);

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
		/// Closes the listening server.
		//------------------------------------------------------//
		void CloseServer();

		//------------------------------------------------------//
		/// True if server is listening :eyes:
		/// @return Bool				True if the server is listening.
		//------------------------------------------------------//
		Bool Closed();

		//------------------------------------------------------//
		/// Returns the position vector of the currently active client.
		/// @return Vector32			The position vector.
		//------------------------------------------------------//
		Vector32 GetPositionVec();

		//------------------------------------------------------//
		/// Returns the rotation vector of the currently active client.
		/// @return Vector32			The rotation vector.
		//------------------------------------------------------//
		Vector32 GetRotationVec();

		//------------------------------------------------------//
		/// Returns the current field of view (fov) of the currently active client.
		/// @return Float32				The Field of View (FoV).
		//------------------------------------------------------//
		Float32 GetFov();

	private:
		//------------------------------------------------------//
		/// Parses a 4 byte float in with little endianess from a char array.
		/// @param[in] data				The char array.
		/// @param[in] offset			The offset for the start of the float.
		/// @return Float32				The computed Float value.
		//------------------------------------------------------//
		Float32 FourByteFloatLE(char *data, UInt64 offset);


		std::vector<Client> _clients;
		uWS::Hub _hub;
		char *_host;
		Int32 _port;
		Bool _listening;
		maxon::Spinlock _slock;

		Float32 _xp, _yp, _zp,
			_xr, _yr, _zr,
			_fov;
	};
}

#endif // !HLL_SERVER_H__
