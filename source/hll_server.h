// Copyright (c) 2018 Brett Anthony. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#ifndef HLL_SERVER_H__
#define HLL_SERVER_H__

#include "c4d.h"
#include "maxon/thread.h"
#include "../uWebSockets/src/uWS.h"

namespace HLL
{
	//------------------------------------------------------//
	/// Provides the interface for our server to run off the main thread.
	//------------------------------------------------------//
	class ServerThread : public maxon::ThreadInterfaceTemplate<ServerThread>
	{
		//------------------------------------------------------//
		/// The actual worker for the server.
		//------------------------------------------------------//
		maxon::Result<void> operator ()()
		{

		}
	};
}

#endif // !HLL_SERVER_H__
