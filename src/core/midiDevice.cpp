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
#include "const.h"
#include "kernelMidi.h"
#include "midiDispatcher.h"
#include "midiDevice.h"

namespace giada {
namespace m
{

	MidiDevice::MidiDevice(int index)
		: m_index				(index),
			m_name 				("MIDI"),
			m_portIn			(-1),
		  m_portOut			(-1),
		  m_noNoteOff		(false),
		  m_midiMapPath	(""),
		  m_lastFileMap	(""),
			m_midiIn			(nullptr),
			m_midiOut			(nullptr),
			m_statusIn		(false),
			m_statusOut		(false)
	{
		/* Create MIDI Input */

		m_midiIn = kernelMidi::openInDevice();
		m_midiOut = kernelMidi::openOutDevice();

	}
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */

int MidiDevice::getIndex() const
{
	return m_index;
}

string MidiDevice::getName() const
{
	return m_name;
}

int MidiDevice::getPortIn() const
{
	return m_portIn;
}
int MidiDevice::getPortOut() const
{
	return m_portOut;
}
bool MidiDevice::getNoNoteOff() const
{
	return m_noNoteOff;
}

void MidiDevice::setName(string name)
{
	m_name = name;
}

void MidiDevice::setPortIn(int port)
{
	kernelMidi::closeInPort(m_midiIn);
	m_portIn = port;
	kernelMidi::openInPort(m_midiIn, port, this);
}

void MidiDevice::setPortOut(int port)
{
	kernelMidi::closeOutPort(m_midiOut);
	m_portOut = port;
	kernelMidi::openOutPort(m_midiOut, port);
}

void MidiDevice::setNoNoteOff(bool no)
{
	m_noNoteOff = no;
}

unsigned MidiDevice::getInPortCount()
{
		return m_midiIn->getPortCount();
}

unsigned MidiDevice::getOutPortCount()
{
		return m_midiOut->getPortCount();
}

string MidiDevice::getInPortName(unsigned p)
{
	try { return m_midiIn->getPortName(p); }
	catch (RtMidiError &error) { return ""; }
}

string MidiDevice::getOutPortName(unsigned p)
{
	try { return m_midiOut->getPortName(p); }
	catch (RtMidiError &error) { return ""; }
}

}} // giada::m::
