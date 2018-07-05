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


#ifndef G_MIDI_DEVICE_H
#define G_MIDI_DEVICE_H

#ifdef G_OS_MAC
	#include <RtMidi.h>
#else
	#include <rtmidi/RtMidi.h>
#endif


#include <string>
using std::string;

namespace giada {
namespace m
{
class MidiDevice
{
public:

	MidiDevice(int index);

	int getIndex() const;
	string getName() const;
	int getPortIn() const;
	int getPortOut() const;
	bool getNoNoteOff() const;

	void setName(string name);
	void setPortIn(int port);
	void setPortOut(int port);
	void setNoNoteOff(bool no);

	unsigned getInPortCount();
	unsigned getOutPortCount();

	string getInPortName(unsigned p);
	string getOutPortName(unsigned p);

private:

	void callback(double t, std::vector<unsigned char>* msg, void* data);

	int		 m_index;
	string m_name;
	int    m_portIn;
	int    m_portOut;
	bool   m_noNoteOff;
	string m_midiMapPath;
	string m_lastFileMap;

	RtMidiIn*  m_midiIn;
	RtMidiOut* m_midiOut;

	bool m_statusIn;
	bool m_statusOut;
};

}} // giada::m::


#endif
