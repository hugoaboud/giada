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


#ifndef G_MIDI_DEVICE_HANDLER_H
#define G_MIDI_DEVICE_HANDLER_H

#include <vector>
#include "midiDevice.h"

namespace giada {
namespace m {
namespace mdh
{

extern std::vector<MidiDevice*> midiDevices;

/* addMidiDevice
Add a new midi device into stack. */

MidiDevice* addMidiDevice();

/* deleteMidiDevice
Completely removes a midi device from the stack. */

bool deleteMidiDevice(MidiDevice* dev);

/* getMidiDeviceCount
Get how many devices are on stack. */
unsigned getMidiDeviceCount();

/* getMidiDeviceByIndex
Returns midi device with given index 'i'. */

MidiDevice* getMidiDeviceByIndex(int i);

}}}  // giada::m::mh::


#endif
