/* -----------------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * -----------------------------------------------------------------------------
 *
 * Copyright (C) G_GUI_UNIT10-G_GUI_UNIT17 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * -------------------------------------------------------------------------- */


#include <FL/Fl_Menu_Button.H>
#include "../../../../core/const.h"
#include "../../../../core/graphics.h"
#include "../../../../core/midiChannel.h"
#include "../../../../utils/gui.h"
#include "../../../../utils/string.h"
#include "../../../../glue/channel.h"
#include "../../../../glue/io.h"
#include "../../../../glue/recorder.h"
#include "../../../dialogs/gd_mainWindow.h"
#include "../../../dialogs/channelNameInput.h"
#include "../../../dialogs/gd_actionEditor.h"
#include "../../../dialogs/gd_warnings.h"
#include "../../../dialogs/gd_keyGrabber.h"
#include "../../../dialogs/pluginList.h"
#include "../../../dialogs/midiIO/midiInputChannel.h"
#include "../../../dialogs/midiIO/midiOutputMidiCh.h"
#include "../../basics/boxtypes.h"
#include "../../basics/idButton.h"
#include "../../basics/statusButton.h"
#include "../../basics/dial.h"
#include "column.h"
#include "midiChannelButton.h"
#include "midiChannel.h"
#include "channelStatus.h"
#include "channelMode.h"
#include "channelRecMode.h"

extern gdMainWindow* G_MainWin;


using std::string;


namespace
{
enum class Menu
{
	INPUT_MONITOR = 0,
	EDIT_ACTIONS,
	CLEAR_ACTIONS,
	CLEAR_ACTIONS_ALL,
	__END_CLEAR_ACTION_SUBMENU__,
	SETUP_KEYBOARD_INPUT,
	SETUP_MIDI_INPUT,
	SETUP_MIDI_OUTPUT,
	RESIZE,
	RESIZE_H1,
	RESIZE_H2,
	RESIZE_H3,
	RESIZE_H4,
	__END_RESIZE_SUBMENU__,
	RENAME_CHANNEL,
	CLONE_CHANNEL,
	DELETE_CHANNEL
};


/* -------------------------------------------------------------------------- */


void menuCallback(Fl_Widget* w, void* v)
{
	using namespace giada;

	geMidiChannel* gch = static_cast<geMidiChannel*>(w);
	Menu selectedItem = (Menu) (intptr_t) v;

	switch (selectedItem)
	{
		case Menu::INPUT_MONITOR: {
			c::channel::toggleInputMonitor(gch->ch);
			break;
		}
		case Menu::CLEAR_ACTIONS:
		case Menu::__END_CLEAR_ACTION_SUBMENU__:
		case Menu::RESIZE:
		case Menu::__END_RESIZE_SUBMENU__:
			break;
		case Menu::EDIT_ACTIONS:
			gu_openSubWindow(G_MainWin, new gdActionEditor(gch->ch), WID_ACTION_EDITOR);
			break;
		case Menu::CLEAR_ACTIONS_ALL:
			c::recorder::clearAllActions(gch);
			break;
		case Menu::SETUP_KEYBOARD_INPUT:
			gu_openSubWindow(G_MainWin, new gdKeyGrabber(gch->ch), 0);
			break;
		case Menu::SETUP_MIDI_INPUT:
			gu_openSubWindow(G_MainWin, new gdMidiInputChannel(gch->ch), 0);
			break;
		case Menu::SETUP_MIDI_OUTPUT:
			gu_openSubWindow(G_MainWin,
				new gdMidiOutputMidiCh(static_cast<MidiChannel*>(gch->ch)), 0);
			break;
		case Menu::RESIZE_H1:
			gch->changeSize(G_GUI_CHANNEL_H_1);
			static_cast<geColumn*>(gch->parent())->repositionChannels();
			break;
		case Menu::RESIZE_H2:
			gch->changeSize(G_GUI_CHANNEL_H_2);
			static_cast<geColumn*>(gch->parent())->repositionChannels();
			break;
		case Menu::RESIZE_H3:
			gch->changeSize(G_GUI_CHANNEL_H_3);
			static_cast<geColumn*>(gch->parent())->repositionChannels();
			break;
		case Menu::RESIZE_H4:
			gch->changeSize(G_GUI_CHANNEL_H_4);
			static_cast<geColumn*>(gch->parent())->repositionChannels();
			break;
		case Menu::CLONE_CHANNEL:
			c::channel::cloneResourceChannel((ResourceChannel*)gch->ch);
			break;
		case Menu::RENAME_CHANNEL:
			gu_openSubWindow(G_MainWin, new gdChannelNameInput(gch->ch), WID_SAMPLE_NAME);
			break;
		case Menu::DELETE_CHANNEL:
			c::channel::deleteChannel(gch->ch);
			break;
	}
}

}; // {namespace}


/* -------------------------------------------------------------------------- */


geMidiChannel::geMidiChannel(int X, int Y, int W, int H, MidiChannel* ch)
	: geResourceChannel(X, Y, W, H, G_CHANNEL_MIDI, ch)
{
	begin();

	button      = new geIdButton(x(), y(), G_GUI_UNIT, G_GUI_UNIT, "", channelStop_xpm, channelPlay_xpm);
	arm         = new geButton(button->x()+button->w()+4, y(), G_GUI_UNIT, G_GUI_UNIT, "", armOff_xpm, armOn_xpm);
	status      = new geChannelStatus(arm->x()+arm->w()+4, y(), G_GUI_UNIT, H, ch);
	mainButton  = new geMidiChannelButton(status->x()+status->w()+4, y(), G_GUI_UNIT, H, "-- MIDI --");
	modeBox     = new geChannelMode(mainButton->x()+mainButton->w()+4, y(), G_GUI_UNIT, G_GUI_UNIT, ch);
	recModeBox  = new geChannelRecMode(modeBox->x()+modeBox->w()+4, y(), G_GUI_UNIT, G_GUI_UNIT, ch);
	mute        = new geButton(recModeBox->x()+recModeBox->w()+4, y(), G_GUI_UNIT, G_GUI_UNIT, "", muteOff_xpm, muteOn_xpm);
	solo        = new geButton(mute->x()+mute->w()+4, y(), G_GUI_UNIT, G_GUI_UNIT, "", soloOff_xpm, soloOn_xpm);

#if defined(WITH_VST)
	fx         = new geStatusButton(solo->x()+solo->w()+4, y(), G_GUI_UNIT, G_GUI_UNIT, fxOff_xpm, fxOn_xpm);
	vol        = new geDial(fx->x()+fx->w()+4, y(), G_GUI_UNIT, G_GUI_UNIT);
#else
	vol        = new geDial(solo->x()+solo->w()+4, y(), G_GUI_UNIT, G_GUI_UNIT);
#endif

	end();

	resizable(mainButton);

	update();

	button->callback(cb_button, (void*)this);
	button->when(FL_WHEN_CHANGED);   // do callback on keypress && on keyrelease

	arm->callback(cb_arm, (void*)this);

#ifdef WITH_VST
	fx->callback(cb_openFxWindow, (void*)this);
#endif

	mute->type(FL_TOGGLE_BUTTON);
	mute->callback(cb_mute, (void*)this);

	solo->type(FL_TOGGLE_BUTTON);
	solo->callback(cb_solo, (void*)this);

	mainButton->callback(cb_openMenu, (void*)this);

	vol->callback(cb_changeVol, (void*)this);

	ch->guiChannel = this;

	changeSize(H);  // Update size dynamically
}


/* -------------------------------------------------------------------------- */

void geMidiChannel::cb_openMenu(Fl_Widget* v, void* p) { ((geMidiChannel*)p)->cb_openMenu(); }

/* -------------------------------------------------------------------------- */

void geMidiChannel::cb_openMenu()
{
	Fl_Menu_Item rclick_menu[] = {
		{"Input monitor",            0, menuCallback, (void*) Menu::INPUT_MONITOR,
			FL_MENU_TOGGLE | FL_MENU_DIVIDER | (ch->inputMonitor ? FL_MENU_VALUE : 0)},
		{"Edit actions...", 0, menuCallback, (void*) Menu::EDIT_ACTIONS},
		{"Clear actions",   0, menuCallback, (void*) Menu::CLEAR_ACTIONS, FL_SUBMENU},
			{"All",           0, menuCallback, (void*) Menu::CLEAR_ACTIONS_ALL},
			{0},
		{"Setup keyboard input...", 0, menuCallback, (void*) Menu::SETUP_KEYBOARD_INPUT},
		{"Setup MIDI input...",     0, menuCallback, (void*) Menu::SETUP_MIDI_INPUT},
		{"Setup MIDI output...",    0, menuCallback, (void*) Menu::SETUP_MIDI_OUTPUT},
		{"Resize",    0, menuCallback, (void*) Menu::RESIZE, FL_SUBMENU},
			{"Normal",  0, menuCallback, (void*) Menu::RESIZE_H1},
			{"Medium",  0, menuCallback, (void*) Menu::RESIZE_H2},
			{"Large",   0, menuCallback, (void*) Menu::RESIZE_H3},
			{"X-Large", 0, menuCallback, (void*) Menu::RESIZE_H4},
			{0},
		{"Rename channel",  0, menuCallback, (void*) Menu::RENAME_CHANNEL},
		{"Clone channel",  0, menuCallback, (void*) Menu::CLONE_CHANNEL},
		{"Delete channel", 0, menuCallback, (void*) Menu::DELETE_CHANNEL},
		{0}
	};

	/* No 'clear actions' if there are no actions. */

	if (!ch->hasActions)
		rclick_menu[(int)Menu::CLEAR_ACTIONS].deactivate();

	Fl_Menu_Button* b = new Fl_Menu_Button(0, 0, 100, 50);
	b->box(G_CUSTOM_BORDER_BOX);
	b->textsize(G_GUI_FONT_SIZE_BASE);
	b->textcolor(G_COLOR_LIGHT_2);
	b->color(G_COLOR_GREY_2);

	const Fl_Menu_Item* m = rclick_menu->popup(Fl::event_x(), Fl::event_y(), 0, 0, b);
	if (m)
		m->do_callback(this, m->user_data());
	return;
}


/* -------------------------------------------------------------------------- */


void geMidiChannel::refresh()
{
	if (!mainButton->visible()) // mainButton invisible? status too (see below)
		return;
	setColorsByStatus(((ResourceChannel*)ch)->status, ((ResourceChannel*)ch)->recStatus);
	status->redraw(); // status invisible? sampleButton too (see below)
	mainButton->redraw();
}


/* -------------------------------------------------------------------------- */


void geMidiChannel::reset()
{
	mainButton->setDefaultMode("-- MIDI --");
	mainButton->redraw();
	status->redraw();
}


/* -------------------------------------------------------------------------- */


void geMidiChannel::update()
{
	const MidiChannel* mch = static_cast<const MidiChannel*>(ch);

	string label;
	if (mch->name.empty())
		label = "-- MIDI --";
	else
		label = mch->name.c_str();

	if (mch->midiOut)
		label += " (ch " + gu_iToString(mch->midiOutChan + 1) + " out)";

	mainButton->label(label.c_str());

	vol->value(mch->volume);
	mute->value(mch->mute);
	solo->value(mch->solo);

	mainButton->setKey(mch->key);

	arm->value(mch->armed || ((MidiChannel*)mch)->isRecording());
	arm->redraw();

	modeBox->value(mch->mode);
	modeBox->redraw();

	recModeBox->value(mch->recMode);
	recModeBox->redraw();

#ifdef WITH_VST
	fx->status = mch->plugins.size() > 0;
	fx->redraw();
#endif
}


/* -------------------------------------------------------------------------- */


void geMidiChannel::resize(int X, int Y, int W, int H)
{
	geChannel::resize(X, Y, W, H);

	arm->hide();
	modeBox->hide();
	recModeBox->hide();
#ifdef WITH_VST
	fx->hide();
#endif

	if (w() > BREAK_ARM)
		arm->show();
#ifdef WITH_VST
	if (w() > BREAK_FX)
		fx->show();
#endif

	if (w() > BREAK_MODE_BOX)
		modeBox->show();
	if (w() > BREAK_MODE_BOX)
			recModeBox->show();

	packWidgets();
}
