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


#include <string>
#include "../../../core/const.h"
#ifdef G_OS_MAC
	#include <RtMidi.h>
#else
	#include <rtmidi/RtMidi.h>
#endif
#include "../../../core/conf.h"
#include "../../../core/midiDeviceHandler.h"
#include "../../../core/midiMapConf.h"
#include "../../../core/kernelMidi.h"
#include "../../../glue/midi.h"
#include "../../../utils/gui.h"
#include "../basics/box.h"
#include "../basics/boxtypes.h"
#include "../basics/button.h"
#include "../basics/choice.h"
#include "../basics/check.h"
#include "tabMidi.h"
#include "midiDevice.h"


using std::string;
using namespace giada::m;


geTabMidi::geTabMidi(int X, int Y, int W, int H)
	: Fl_Group(X, Y, W, H, "MIDI")
{
	begin();
	system	   = new geChoice(x()+w()-250, y()+9, 250, 20, "System");
	deviceList = new Fl_Scroll(16, system->y()+system->h()+8, W, 140);
	deviceList->begin();
		refreshDeviceList();
	deviceList->end();
	sync	     = new geChoice(x()+w()-250, deviceList->y()+deviceList->h()+8, 250, 20, "Sync");
	new geBox(x(), sync->y()+sync->h()+8, w(), h()-256, "Restart Giada for the changes to take effect.");
	end();

	labelsize(G_GUI_FONT_SIZE_BASE);
	selection_color(G_COLOR_GREY_4);

	system->callback(cb_changeSystem, (void*)this);
	fetchSystems();

	deviceList->type(Fl_Scroll::VERTICAL);
	deviceList->scrollbar.color(G_COLOR_GREY_2);
	deviceList->scrollbar.selection_color(G_COLOR_GREY_4);
	deviceList->scrollbar.labelcolor(G_COLOR_LIGHT_1);
	deviceList->scrollbar.slider(G_CUSTOM_BORDER_BOX);

	sync->add("(disabled)");
	sync->add("MIDI Clock (master)");
	sync->add("MTC (master)");
	if      (conf::midiSync == MIDI_SYNC_NONE)
		sync->value(0);
	else if (conf::midiSync == MIDI_SYNC_CLOCK_M)
		sync->value(1);
	else if (conf::midiSync == MIDI_SYNC_MTC_M)
		sync->value(2);

	systemInitValue = system->value();
}

/* -------------------------------------------------------------------------- */


void geTabMidi::refreshDeviceList()
{
	/* delete the previous list */

	deviceList->clear();
	deviceList->scroll_to(0, 0);

	/* add new buttons, as many as the midi devies in mdh::midiDevices,
	 * + the 'add new' button. */

	int numDevices = mdh::midiDevices.size();
	for (int i = 0; i < numDevices; i++) {
		MidiDevice *dev = mdh::midiDevices[i];
		geMidiDevice *gdmd     = new geMidiDevice(dev, deviceList->x(), deviceList->y()-deviceList->yposition()+(i*48), w());
		deviceList->add(gdmd);
	}

	int addDeviceY = numDevices == 0 ? 90 : deviceList->y()-deviceList->yposition()+(numDevices*48);
	addDevice = new geButton(deviceList->x()+8, addDeviceY, w()-16, 20, "-- add new MIDI device --");
	addDevice->callback(cb_addDevice, (void*)this);
	deviceList->add(addDevice);

	redraw();
}

/* -------------------------------------------------------------------------- */


void geTabMidi::save()
{
	string text = system->text(system->value());

	if      (text == "ALSA")
		conf::midiSystem = RtMidi::LINUX_ALSA;
	else if (text == "Jack")
		conf::midiSystem = RtMidi::UNIX_JACK;
	else if (text == "Multimedia MIDI")
		conf::midiSystem = RtMidi::WINDOWS_MM;
	else if (text == "OSX Core MIDI")
		conf::midiSystem = RtMidi::MACOSX_CORE;

	if      (sync->value() == 0)
		conf::midiSync = MIDI_SYNC_NONE;
	else if (sync->value() == 1)
		conf::midiSync = MIDI_SYNC_CLOCK_M;
	else if (sync->value() == 2)
		conf::midiSync = MIDI_SYNC_MTC_M;
}


/* -------------------------------------------------------------------------- */


void geTabMidi::fetchSystems()
{
#if defined(__linux__)

	if (kernelMidi::hasAPI(RtMidi::LINUX_ALSA))
		system->add("ALSA");
	if (kernelMidi::hasAPI(RtMidi::UNIX_JACK))
		system->add("Jack");

#elif defined(_WIN32)

	if (kernelMidi::hasAPI(RtMidi::WINDOWS_MM))
		system->add("Multimedia MIDI");

#elif defined (__APPLE__)

	system->add("OSX Core MIDI");

#endif

	switch (conf::midiSystem) {
		case RtMidi::LINUX_ALSA:  system->showItem("ALSA"); break;
		case RtMidi::UNIX_JACK:   system->showItem("Jack"); break;
		case RtMidi::WINDOWS_MM:  system->showItem("Multimedia MIDI"); break;
		case RtMidi::MACOSX_CORE: system->showItem("OSX Core MIDI"); break;
		default: system->value(0); break;
	}
}


/* -------------------------------------------------------------------------- */


void geTabMidi::cb_changeSystem(Fl_Widget *w, void *p) { ((geTabMidi*)p)->__cb_changeSystem(); }
void geTabMidi::cb_addDevice(Fl_Widget *w, void *p) { ((geTabMidi*)p)->__cb_addDevice(); }


/* -------------------------------------------------------------------------- */


void geTabMidi::__cb_changeSystem()
{
	/* if the user changes MIDI device (eg ALSA->JACK) device menu deactivates.
	 * If it returns to the original system, we re-fill the list by
	 * querying kernelMidi. */

	// TODO: do this for every midi device on the list
	if (systemInitValue == system->value()) {
		/*portOut->clear();
		fetchOutPorts();
		portOut->activate();
		portIn->clear();
		fetchInPorts();
		portIn->activate();
		noNoteOff->activate();*/
		sync->activate();
	}
	else {
		/*portOut->deactivate();
		portOut->clear();
		portOut->add("-- restart to fetch device(s) --");
		portOut->value(0);
		portIn->deactivate();
		portIn->clear();
		portIn->add("-- restart to fetch device(s) --");
		portIn->value(0);
		noNoteOff->deactivate();*/
		sync->deactivate();
	}

}

/* -------------------------------------------------------------------------- */


void geTabMidi::__cb_addDevice()
{
	giada::c::midi::addMidiDevice();
}
