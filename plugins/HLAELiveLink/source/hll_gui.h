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
#include "hll_tools.h"
#include "maxon/ioxmlparser.h"
#include "maxon/timer.h"
#include <shared_mutex>

#define HLL_STATUSBAR_POLL_RATE		200
#define HLL_BANNER_WIDTH			400
#define HLL_BANNER_HEIGHT			77

namespace HLL
{
	class Banner;

	//------------------------------------------------------//
	/// Primary GUI for the plugin
	//------------------------------------------------------//
	class Gui : public GeDialog
	{
		virtual ~Gui() { DeleteObj(_banner); }
	public:
		Bool CreateLayout();
		Bool InitValues();
		Bool Command(Int32 id, const BaseContainer &msg);
		Bool CoreMessage(Int32 id, const BaseContainer &msg);
		void DestroyWindow();
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

		void SetUI(Bool enable);
		void ResetFunctionMetrics();
		void DoUpdateCheck();

		Bool StartUpdate(Bool setup_client = true);
		void StopUpdate();

		Bool IsCameraValid();
		void ResetCamera();

		SimpleListView _LVClients;

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
		Bool Execute(BaseDocument *doc, GeDialog* parentManager) override;
	};

	//------------------------------------------------------//
	/// Registers the plugin.
	/// @return Bool			True if successful.
	//------------------------------------------------------//
	Bool RegisterHLL(std::shared_ptr<Tools::UserConfig> userConfig);
}

#endif // !HLL_GUI_H__