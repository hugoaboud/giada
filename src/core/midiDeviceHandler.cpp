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

#include "../utils/log.h"
#include "midiDevice.h"
#include "midiDeviceHandler.h"

using std::vector;

namespace giada {
namespace m {
namespace mdh
{

std::vector<MidiDevice*> midiDevices;

/* ------------------------------------------------------------------------ */

int deviceIndex = 0;

int getNewDeviceIndex()
{
	return deviceIndex++;
}

/* ------------------------------------------------------------------------ */

MidiDevice* addMidiDevice()
{
	MidiDevice* dev = new MidiDevice(getNewDeviceIndex());
	midiDevices.push_back(dev);

	gu_log("[addMidiDevice] device %d added, total=%d\n", dev->getIndex(), midiDevices.size());
	return dev;
}

/* ------------------------------------------------------------------------ */

bool deleteMidiDevice(MidiDevice* dev)
{
	int index = -1;
	for (unsigned i=0; i<midiDevices.size(); i++) {
		if (midiDevices.at(i) == dev) {
			index = i;
			break;
		}
	}
	if (index == -1) {
		gu_log("[deleteMidiDevice] unable to find device %d for deletion!\n", dev->getIndex());
		return false;
	}

	midiDevices.erase(midiDevices.begin() + index);
	return true;
}

unsigned getMidiDeviceCount()
{
	return midiDevices.size();
}

/* ------------------------------------------------------------------------ */

MidiDevice* getMidiDeviceByIndex(int index)
{
	if (index < 0 or (unsigned) index > midiDevices.size()-1) return nullptr;
	return midiDevices.at(index);
}

}}}; // giada::m::mh::
