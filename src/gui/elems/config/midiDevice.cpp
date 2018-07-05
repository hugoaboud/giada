/* -----------------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * -----------------------------------------------------------------------------
 *
 * Copyright (C) 2010-2018 Giovanni A. Zuliani | Monocasual
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

#include "../../../core/conf.h"
#include "../../../core/const.h"
#include "../../../core/midiDevice.h"
#include "../../../core/kernelMidi.h"
#include "../../../core/midiMapConf.h"
#include "../../../glue/midi.h"
#include "../basics/boxtypes.h"
#include "../basics/choice.h"
#include "../basics/check.h"
#include "../basics/idButton.h"
#include "../../dialogs/midiDeviceNameInput.h"
#include "../../dialogs/gd_mainWindow.h"
#include "../../../utils/gui.h"
#include "midiDevice.h"

extern gdMainWindow* G_MainWin;

using namespace giada::m;

geMidiDevice::geMidiDevice(MidiDevice* device, int X, int Y, int W)
	: Fl_Group(X, Y, W, 40), device(device)
{
	begin();
	button  	= new geIdButton(x(), y(), (W-8)*5/21, 20);
	portIn    = new geChoice(button->x()+button->w()+4, y(), (W-8)*8/21, 20);
	portOut   = new geChoice(portIn->x()+portIn->w()+4, y(), (W-8)*8/21, 20);
	noNoteOff = new geCheck(x(), y()+portOut->h()+4, 20, 20, "No NoteOff");
	midiMap	  = new geChoice(x()+W*2/3, y()+portOut->h()+4, W/3, 20, "Midi Map");
	end();

	button->copy_label(device->getName().c_str());
	button->callback(cb_button, (void*)this);
	button->value(0);

	portIn->callback(cb_setPortIn, (void*)this);
	portOut->callback(cb_setPortOut, (void*)this);

	noNoteOff->value(device->getNoNoteOff());

	fetchOutPorts();
	fetchInPorts();
	fetchMidiMaps();
}

/* -------------------------------------------------------------------------- */

void geMidiDevice::fetchInPorts()
{
	if (device->getInPortCount() == 0) {
		portIn->add("- no inputs -");
		portIn->value(0);
		portIn->deactivate();
		noNoteOff->deactivate();
	}
	else {

		portIn->add("(disabled)");

		for (unsigned i=0; i<device->getInPortCount(); i++)
			portIn->add(gu_removeFltkChars(device->getInPortName(i)).c_str());

		portIn->value(conf::midiPortIn+1);    // +1 because midiPortIn=-1 is '(disabled)'
	}
}

/* -------------------------------------------------------------------------- */

void geMidiDevice::fetchOutPorts()
{
	if (device->getOutPortCount() == 0) {
		portOut->add("- no outputs -");
		portOut->value(0);
		portOut->deactivate();
	}
	else {

		portOut->add("(disabled)");

		for (unsigned i=0; i<device->getOutPortCount(); i++)
			portOut->add(gu_removeFltkChars(device->getOutPortName(i)).c_str());

		portOut->value(conf::midiPortOut+1);    // +1 because midiPortOut=-1 is '(disabled)'
	}
}

/* -------------------------------------------------------------------------- */

void geMidiDevice::fetchMidiMaps()
{
	if (midimap::maps.size() == 0) {
		midiMap->add("- no maps -");
		midiMap->value(0);
		midiMap->deactivate();
		return;
	}

	for (unsigned i=0; i<midimap::maps.size(); i++) {
		const char *imap = midimap::maps.at(i).c_str();
		midiMap->add(imap);
		if (conf::midiMapPath == imap)
			midiMap->value(i);
	}

	/* Preselect the 0 midimap if nothing is selected but midimaps exist. */

	if (midiMap->value() == -1 && midimap::maps.size() > 0)
		midiMap->value(0);
}

/* -------------------------------------------------------------------------- */

void geMidiDevice::cb_button		    (Fl_Widget* v, void* p) { ((geMidiDevice*)p)->cb_button(); }
void geMidiDevice::cb_setPortIn     (Fl_Widget* v, void* p) { ((geMidiDevice*)p)->cb_setPortIn(); }
void geMidiDevice::cb_setPortOut		(Fl_Widget* v, void* p) { ((geMidiDevice*)p)->cb_setPortOut(); }
void geMidiDevice::cb_setNoNoteOff	(Fl_Widget* v, void* p) { ((geMidiDevice*)p)->cb_setNoNoteOff(); }
void geMidiDevice::cb_midiMap				(Fl_Widget* v, void* p) { ((geMidiDevice*)p)->cb_midiMap(); }

/* -------------------------------------------------------------------------- */

enum class Menu
{
	RENAME_DEVICE = 0,
	DELETE_DEVICE
};

void deviceButtonCallback(Fl_Widget* w, void* v)
{
	geMidiDevice* gdev = static_cast<geMidiDevice*>(w);
	Menu selectedItem = (Menu) (intptr_t) v;

	switch (selectedItem) {
		case Menu::RENAME_DEVICE:
			gu_openSubWindow(G_MainWin, new gdMidiDeviceNameInput(gdev->device), WID_SAMPLE_NAME);
			break;
		case Menu::DELETE_DEVICE:
			giada::c::midi::deleteMidiDevice(gdev->device);
			break;
	}
}

void geMidiDevice::cb_button()
{
	using namespace giada;

	Fl_Menu_Item rclick_menu[] = {
		{"Rename device", 		0, deviceButtonCallback, (void*) Menu::RENAME_DEVICE},
		{"Delete device", 		0, deviceButtonCallback, (void*) Menu::DELETE_DEVICE},
		{0}
	};

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


void geMidiDevice::cb_setPortIn()
{
	device->setPortIn(portIn->value()-1);
}

/* -------------------------------------------------------------------------- */


void geMidiDevice::cb_setPortOut()
{

}

/* -------------------------------------------------------------------------- */


void geMidiDevice::cb_setNoNoteOff()
{

}

/* -------------------------------------------------------------------------- */


void geMidiDevice::cb_midiMap()
{

}
