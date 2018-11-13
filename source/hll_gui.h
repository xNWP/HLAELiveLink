// Copyright (c) 2018 Brett Anthony. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#ifndef HLL_GUI_H__
#define HLL_GUI_H__

#include "c4d.h"
#include "../res/c4d_symbols.h"

namespace HLL
{
	//------------------------------------------------------//
	/// Primary GUI for the plugin
	//------------------------------------------------------//
	class Gui : public GeDialog
	{
	public:
		Bool CreateLayout();
		Bool InitValues();
		Bool Command(Int32 id, const BaseContainer &msg);
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
	/// @return BOOL		True if successful.
	//------------------------------------------------------//
	Bool RegisterHLL();
}

#endif // !HLL_GUI_H__