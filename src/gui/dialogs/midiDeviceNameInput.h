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


#ifndef GD_MIDI_DEVICE_NAME_INPUT_H
#define GD_MIDI_DEVICE_NAME_INPUT_H


#include "window.h"
#include "../../core/midiDevice.h"
class geInput;
class geButton;

class gdMidiDeviceNameInput : public gdWindow
{
private:

	static void cb_update(Fl_Widget* w, void* p);
	static void cb_cancel(Fl_Widget* w, void* p);
	void cb_update();
	void cb_cancel();

	giada::m::MidiDevice* m_dev;

	geInput* m_name;
	geButton* m_ok;
	geButton* m_cancel;

public:

	gdMidiDeviceNameInput(giada::m::MidiDevice* dev);
	~gdMidiDeviceNameInput();
};

#endif
