// Copyright (c) 2018 Brett Anthony. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#ifndef HLL_GUI_H__
#define HLL_GUI_H__

#include "c4d.h"
#include "../res/c4d_symbols.h"
#include "hll_globals.h"
#include "hll_maths.h"
#include "hll_server.h"
#include "maxon/timer.h"

#define HLL_STATUSBAR_POLL_RATE		200
#define HLL_BANNER_WIDTH			400
#define HLL_BANNER_HEIGHT			77

namespace HLL
{
	class Banner;

	static maxon::ThreadRefTemplate<ServerThread> g_ServerThread;
	static maxon::TimerRef g_UpdateCamera;

	/*	
		The following static variables will be the current setting of the UI,
		which will be preserved between opens/closes of the UI. The C4D API
		disallows initializing Strings on the global scope. For this reason
		we will initialize them in the InitValues() function of the GUI,
		using the Bool g_HLL_Init to avoid resetting the values with each open.
	*/
	static Bool g_HLL_Init = false;
	static String g_HLL_Hostname;
	static String g_HLL_Port;
	static Bool g_HLL_Listening = false;
	static String g_HLL_Mapping;
	static String g_HLL_Camera;
	static String g_HLL_Listen;
	static String g_HLL_PollRate;
	static Bool g_HLL_CheckForUpdates = false;

	static BaseObject *g_HLL_OCamera;
	static Int32 g_HLL_ActiveClient;

	struct HLL_StatusMessage
	{
		HLL_StatusMessage(Int32 duration, const String &msg) : timeleft(duration), message(msg) { }
		Int32 timeleft;
		String message;
	};

	static maxon::BaseArray<HLL_StatusMessage> g_HLL_StatusMessageQueue;

	//------------------------------------------------------//
	/// Primary GUI for the plugin
	//------------------------------------------------------//
	class Gui : public GeDialog
	{
		~Gui() { DeleteObj(_banner); }
	public:
		Bool CreateLayout();
		Bool InitValues();
		Bool Command(Int32 id, const BaseContainer &msg);
		Bool CoreMessage(Int32 id, const BaseContainer &msg);
		void Timer(const BaseContainer &msg);

	private:
		//------------------------------------------------------//
		/// Sets the status bar to the given text for the given amount of time.
		/// If the duration is set to 0, the message will be 'sticky', meaning it will stay until the status
		/// bar is cleared with ClearStatusText(). If multiple sticky messages are added, the last 'sticky' message
		/// added will be shown. Any non-'sticky' messages will be shown before sticky messages are shown.
		/// @param[in] duration			The time in milliseconds that the message should be shown for.
		/// @param[in] msg				The message to be shown.
		//------------------------------------------------------//
		void SetStatusText(Int32 duration, const String &msg);

		void ClearStatusText();

		//------------------------------------------------------//
		/// Enable or disable gadgets related to the servers listening.
		/// @param[in] val				Whether to disable or enable the gadgets.
		//------------------------------------------------------//
		void SetListenGadgets(const Bool &val);

		SimpleListView _lvclients;

		class Banner : public GeUserArea
		{
		public:
			~Banner();
			Bool GetMinSize(Int32 &w, Int32 &h) { w = HLL_BANNER_WIDTH; h = HLL_BANNER_HEIGHT; return true; }
			void DrawMsg(Int32 x1, Int32 y1, Int32 x2, Int32 y2, const BaseContainer &msg);

		private:
			BaseBitmap *bmp;
		};

		Banner *_banner;
	};

	//------------------------------------------------------//
	/// Provides the CommandData instance for the GUI.
	//------------------------------------------------------//
	class GuiCommand : public CommandData
	{
	public:
		Bool Execute(BaseDocument *doc);

	private:
		Gui *oGui;
	};

	//------------------------------------------------------//
	/// Registers the plugin.
	/// @return Bool			True if successful.
	//------------------------------------------------------//
	Bool RegisterHLL();
}

#endif // !HLL_GUI_H__