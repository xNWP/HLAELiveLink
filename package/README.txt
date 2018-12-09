== PREREQUISITES ==
+ Cinema 4D R20 or newer.
+ Half-Life Advanced Effects (latest version) -- https://www.advancedfx.org/download/

== INSTALLATION ==
Delete any existing HLAELiveLink Installations.

Copy the 'HLAELiveLink' folder into the Cinema 4D Plugins folder.

	C:\Program Files\MAXON\Cinema 4D\plugins
		OR
	C:\Users\$USERNAME$\AppData\Roaming\MAXON\Cinema 4D\plugins
	*You may need to create the plugins folder
	
Start Cinema 4D and see if the plugin has registered by looking for its
	entry in plugins > HLAELiveLink.
	
== USAGE ==
- Start Listen: Start the websocket server (once started it cannot be restarted without Cinema 4D also restarting).
- Stop Listen: Ensure that you stop the server before closing Cinema 4D otherwise you may get an error message (which you can safely ignore).
- Hostname: The IPv4 address for the websocket to listen on, cannot contain paths (i.e. 127.0.0.1/mypath).
- Port: The port for the websocket to listen on.
- Update Rate: How many times per second the camera data is updated (whether in Cinema 4D or CS:GO). Higher values
	may cause instability, experiment at own risk before using in any live capacity.
- Copy Connect Command To Clipboard: A convenience function to connect to the server from CS:GO.
- Set Camera: Select a camera in Cinema 4D, then click this command to use that camera to transmit/receive camera data.
- Mapping: Flip between Game and Cinema 4D to determine which camera is the slave to the other.
- Client List: List's the clients, click their checkbox to make them the 'active client' (the one we send/receive data from).
	Click their name to rename them for organization. Click disconnect to disconnect the client from the server.
- Status Bar: Will provide information about the current state of the program.
- Update (menu): Check for updates and enable/disable automatic update checking of the plugin.


== CREDITS ==

+ The HLAELiveLink Team
Brett 'xNWP' Anthony
	Lead Programmer
	Since November 2018
	https://twitter.com/ThatNWP

	MIT License

	Copyright (c) 2018 Brett Anthony

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.

== EXTERNAL LIBRARIES ==

+ uWebSockets 0.14
	Used for WebSocket functionality
	https://github.com/uNetworking/uWebSockets
	
	License omitted due to size, available at: http://www.apache.org/licenses/LICENSE-2.0