== PREREQUISITES ==
+ Cinema 4D R21 or newer.
+ Half-Life Advanced Effects (latest version) -- https://www.advancedfx.org/download/

== INSTALLATION ==
Delete any existing HLAELiveLink Installations.

Copy the 'HLAELiveLink' folder into the Cinema 4D Plugins folder.

	C:\Program Files\MAXON\Cinema 4D\plugins
		OR
	C:\Users\$USERNAME$\AppData\Roaming\MAXON\Cinema 4D\plugins
	*You may need to create the plugins folder
	
Start Cinema 4D and see if the plugin loaded by
looking for its	entry in plugins > HLAELiveLink.
	
== USAGE ==
- Start Listen : Start the websocket server.
- Stop Listen  : Stops the websocket server.
- Hostname     : The IPv4 address for the websocket to listen on,
                   cannot contain paths (i.e. 127.0.0.1/mypath).
- Port         : The port for the websocket to listen on.
- f-time       : Function-time; Time it took to send/receive last update.
- avg-time     : Rolling average function-time.
- max-time     : Maximum time f-time can be before we can't send/receive
                   updates quickly enough. Cinema 4D only tells the plugin the
				   max time if we happen to exceed it, so you won't see a value
				   here until that happens, refer to the average time to decide
				   whether or not you should lower the update rate.
- Update Rate  : How many times per second the camera data is sent/received.
                   Higher values may cause instability, experiment at
				   own risk before using in any live capacity.
- Glob. Pos/Rot: When selected, the camera data will be applied (for receive)
                   or sampled (for sending) in global space. Otherwise it
				   will be sampled in local space, this setting is only
				   needed if you have your camera as the child of some null.
- Offset Rotn. : Adds an offset rotation to the camera in Cinema 4D/CS:GO. 
- Set Camera   : Select a camera in Cinema 4D, then click this button to use
                   that camera to transmit/receive camera data.
- Mapping      : When set to Cinema 4D, the selected camera will mimic the
                   game camera, when set to Game, the game camera will mimic
				   the selected Cinema 4D camera.
- Client List  : List's all the connected clients, click a clients checkbox
                   to make them the active client (note: you may only have
				   one active client at a time). Click a clients name to
				   rename them, and click disconnect to disconnect that
				   client from the server.
- Status Bar   : Will provide information about the current state of the plugin.
- Update (menu): Check for updates or enable/disable
                   automatic update checking for the plugin.

== CREDITS ==

+ The HLAELiveLink Team
Brett 'xNWP' Anthony
	Lead Programmer
	Since November 2018
	https://twitter.com/ThatNWP

	MIT License

	Copyright (c) 2021 Brett Anthony

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files
	(the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify,
	merge, publish, distribute, sublicense, and/or sell	copies of the
	Software, and to permit persons to whom the Software is furnished
	to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

== EXTERNAL LIBRARIES ==

+ uWebSockets 19.2.0
	Used for WebSocket functionality
	https://github.com/uNetworking/uWebSockets
	
	License omitted due to size, available at:
	http://www.apache.org/licenses/LICENSE-2.0