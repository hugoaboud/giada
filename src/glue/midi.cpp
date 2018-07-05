/* -----------------------------------------------------------------------------
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

#include <FL/Fl.H>
#include "../gui/dialogs/gd_mainWindow.h"
#include "../gui/dialogs/gd_config.h"
#include "../gui/dialogs/gd_warnings.h"
#include "../gui/elems/config/tabMidi.h"
#include "../core/midiDeviceHandler.h"
#include "../core/const.h"
#include "../utils/gui.h"
#include "midi.h"

extern gdMainWindow* G_MainWin;

using std::string;

namespace giada {
namespace c     {
namespace midi {

/* -------------------------------------------------------------------------- */

m::MidiDevice* addMidiDevice() {
  m::MidiDevice* ch    = m::mdh::addMidiDevice();

	gdConfig* config = static_cast<gdConfig*>(gu_getSubwindow(G_MainWin, WID_CONFIG));
	if (config) {
		Fl::lock();
    config->tabMidi->refreshDeviceList();
		Fl::unlock();
	}

	return ch;
}

/* -------------------------------------------------------------------------- */

void deleteMidiDevice(m::MidiDevice* dev, bool warn) {
	using namespace giada::m;

	if (warn) {
  	if (!gdConfirmWin("Warning", "Delete MIDI Device: are you sure?"))
  		return;
	}
  m::mdh::deleteMidiDevice(dev);

  gdConfig* config = static_cast<gdConfig*>(gu_getSubwindow(G_MainWin, WID_CONFIG));
	if (config) {
		Fl::lock();
    config->tabMidi->refreshDeviceList();
		Fl::unlock();
	}
}

/* -------------------------------------------------------------------------- */

void setName(m::MidiDevice* dev, string name) {
	dev->setName(name);
  gdConfig* config = static_cast<gdConfig*>(gu_getSubwindow(G_MainWin, WID_CONFIG));
	if (config) {
		Fl::lock();
    config->tabMidi->refreshDeviceList();
		Fl::unlock();
	}
}

}}}; // giada::c::midi::
