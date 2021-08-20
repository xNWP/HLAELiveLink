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
#ifdef MAXON_TARGET_WINDOWS
#include "App.h"
#include "libusockets.h"
#include "maxon/utilities/undef_win_macros.h"
#endif
#pragma warning(pop)
#include "maxon/conditionvariable.h"

#define HLL_EVMSG_CLIENT_CONNECT			1
#define HLL_EVMSG_CLIENT_DISCONNECT			2
#define HLL_EVMSG_LISTEN_SUCCESS			3
#define HLL_EVMSG_LISTEN_FAILED				4
#define HLL_EVMSG_DATASTART					5
#define HLL_EVMSG_DATASTOP					6

#include "hll_tools.h"

namespace HLL
{
	//------------------------------------------------------//
	/// Provides information about the client as well as their socket.
	//------------------------------------------------------//
	struct Client
	{
		Client(String name, void* socket) : _name(name), _socket(socket) {}
		String _name;
		void* _socket;
	};

	struct ClientData { };

	//------------------------------------------------------//
	/// Provides the interface for our server to run off the main thread.
	//------------------------------------------------------//
	class ServerThread : public maxon::ThreadInterfaceTemplate<ServerThread>
	{
	public:
		//------------------------------------------------------//
		/// Returns the singleton.
		/// @return ServerThread&	A reference to the server thread.
		//------------------------------------------------------//
		static ServerThread& GetInstance();

		//------------------------------------------------------//
		/// The actual worker for the server.
		//------------------------------------------------------//
		maxon::Result<void> operator()();

		//------------------------------------------------------//
		/// Send a command to a client by index (ServerThread owns the clients).
		/// @param[in] index			The client by index to send the command.
		/// @param[in] command			The command to send.
		//------------------------------------------------------//
		void SendClient(Int32 index, const String& command);

		//------------------------------------------------------//
		/// Send a command over the socket.
		/// @param[in] command			The command to send.
		/// @param[in] ws				The socket to send the command to.
		//------------------------------------------------------//
		void SendSocket(const String& command, void* ws);

		//------------------------------------------------------//
		/// Returns a reference to the clients.
		/// @return std::vector<Client>	The clients.
		//------------------------------------------------------//
		std::vector<Client> GetClients();

		//------------------------------------------------------//
		/// Rename a client.
		/// @param[in] index			The index of the client.
		/// @param[in] str				The new name of the client.
		//------------------------------------------------------//
		void RenameClient(Int32 index, const String& str);

		//------------------------------------------------------//
		/// Gracefully disconnect a client.
		/// @param[in] index			The index of the client.
		//------------------------------------------------------//
		void DisconnectClient(Int32 index);

		//------------------------------------------------------//
		/// Starts the listening server.
		/// @param[in] host				Hostname.
		/// @param[in] port				Port.
		//------------------------------------------------------//
		void StartServer(const String& host, Int32 port);

		//------------------------------------------------------//
		/// Closes the listening server.
		/// @return Bool				True if able to close the listen server.
		//------------------------------------------------------//
		Bool CloseServer();

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

		void EventHandled();

		const maxon::Char* GetName() const { return "HLAELiveLink-ServerThread"; }

	private:
		//------------------------------------------------------//
		/// Parses a 4 byte float in with little endianess from a char array.
		/// @param[in] data				The char array.
		/// @param[in] offset			The offset for the start of the float.
		/// @return Float32				The computed Float value.
		//------------------------------------------------------//
		Float32 FourByteFloatLE(std::string_view data, UInt64 offset);

		std::vector<Client> _clients;
		us_listen_socket_t* _listen_socket;
		std::mutex _dataMutex;
		String _host;
		int _port;
		maxon::ConditionVariableRef _guiHandlingMessage;

		Float32 _xp, _yp, _zp,
			_xr, _yr, _zr,
			_fov;
	};
}

#endif // !HLL_SERVER_H__
