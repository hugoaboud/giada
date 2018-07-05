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


#ifndef GE_MIDI_DEVICE_H
#define GE_MIDI_DEVICE_H

#include <FL/Fl_Group.H>
#include "../../../core/midiDevice.h"

class geIdButton;
class geChoice;
class geCheck;

using namespace giada::m;

class geMidiDevice : public Fl_Group
{
private:

	static void cb_button(Fl_Widget *v, void *p);
	static void cb_setPortIn(Fl_Widget *v, void *p);
	static void cb_setPortOut(Fl_Widget *v, void *p);
	static void cb_setNoNoteOff(Fl_Widget *v, void *p);
	static void cb_midiMap(Fl_Widget *v, void *p);

	void cb_button();
	void cb_setPortIn();
	void cb_setPortOut();
	void cb_setNoNoteOff();
	void cb_midiMap();

	void fetchOutPorts();
	void fetchInPorts();
	void fetchMidiMaps();

public:

	MidiDevice *device;

	geIdButton *button;
	geChoice *portIn;
	geChoice *portOut;
	geCheck  *noNoteOff;
	geChoice *midiMap;

	geMidiDevice(MidiDevice *device, int X, int Y, int W);

	void save();
};


#endif
