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


#include "const.h"
#include "../utils/log.h"
#include "midiDispatcher.h"
#include "midiMapConf.h"
#include "kernelMidi.h"


using std::string;
using std::vector;


namespace giada {
namespace m {
namespace kernelMidi
{
namespace
{
bool status = false;
int api = 0;
RtMidiOut* midiOut = nullptr;
//RtMidiIn*  midiIn  = nullptr;
unsigned numOutPorts = 0;
unsigned numInPorts  = 0;


static void callback(double t, std::vector<unsigned char>* msg, void* data)
{
	if (msg->size() < 3) {
		//gu_log("[KM] MIDI received - unknown signal - size=%d, value=0x", (int) msg->size());
		//for (unsigned i=0; i<msg->size(); i++)
		//	gu_log("%X", (int) msg->at(i));
		//gu_log("\n");
		return;
	}
	midiDispatcher::dispatch((MidiDevice*) data, msg->at(0), msg->at(1), msg->at(2));
}


/* -------------------------------------------------------------------------- */


void sendMidiLightningInitMsgs()
{
	for(unsigned i=0; i<midimap::initCommands.size(); i++) {
		midimap::message_t msg = midimap::initCommands.at(i);
		if (msg.value != 0x0 && msg.channel != -1) {
			gu_log("[KM] MIDI send (init) - Channel %x - Event 0x%X\n", msg.channel, msg.value);
			send(msg.value | MIDI_CHANS[msg.channel]);
		}
	}
}

}; // {anonymous}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


void setApi(int _api)
{
	api = _api;
	gu_log("[KM] using system 0x%x\n", api);
}

/* -------------------------------------------------------------------------- */

RtMidiOut *openOutDevice()
{
	RtMidiOut *midiOut = nullptr;

	try {
		midiOut = new RtMidiOut((RtMidi::Api) api, "Giada MIDI output");
	}
	catch (RtMidiError &error) {
		gu_log("[KM] MIDI out device error: %s\n", error.getMessage().c_str());
	}

	/* print input ports */

	if (midiOut != nullptr) {
		unsigned numOutPorts = midiOut->getPortCount();
		gu_log("[KM] %d output MIDI ports found\n", numOutPorts);
		for (unsigned i=0; i<numOutPorts; i++)
			gu_log("  %d) %s\n", i, midiOut->getPortName(i).c_str());
	}

	return midiOut;
}

int openOutPort(RtMidiOut *device, int port)
{
	/* try to open a port, if enabled */

	if (port >= 0 && (unsigned) port < device->getPortCount()) {
		try {
			device->openPort(port, device->getPortName(port));
			gu_log("[KM] MIDI out port %d open\n", port);

			/* TODO - it shold send midiLightning message only if there is a map loaded
			and available in midimap:: */

			//sendMidiLightningInitMsgs();
			return 1;
		}
		catch (RtMidiError &error) {
			gu_log("[KM] unable to open MIDI out port %d: %s\n", port, error.getMessage().c_str());
			status = false;
			return 0;
		}
	}
	else
		return 2;
}

int closeOutPort(RtMidiOut *device)
{
	try {
		device->closePort();
		gu_log("[KM] MIDI out closed\n");
		return 1;
	}
	catch (RtMidiError &error) {
		gu_log("[KM] unable to close MIDI out: %s\n", error.getMessage().c_str());
	}
	return 0;
}


/* -------------------------------------------------------------------------- */

RtMidiIn *openInDevice()
{
	RtMidiIn *midiIn = nullptr;

	try {
		midiIn = new RtMidiIn((RtMidi::Api) api, "Giada MIDI input");
	}
	catch (RtMidiError &error) {
		gu_log("[KM] MIDI in device error: %s\n", error.getMessage().c_str());
	}

	/* print input ports */

	if (midiIn != nullptr) {
		unsigned numInPorts = midiIn->getPortCount();
		gu_log("[KM] %d input MIDI ports found\n", numInPorts);
		for (unsigned i=0; i<numInPorts; i++)
			gu_log("  %d) %s\n", i, midiIn->getPortName(i).c_str());
	}

	return midiIn;
}

int openInPort(RtMidiIn *device, int port, MidiDevice *dev)
{

	/* try to open a port, if enabled */

	if (port >= 0 && (unsigned) port < device->getPortCount()) {
		try {
			device->openPort(port, device->getPortName(port));
			device->ignoreTypes(true, false, true); // ignore all system/time msgs, for now
			gu_log("[KM] MIDI in port %d open\n", port);
			device->setCallback(callback, dev);
			return 1;
		}
		catch (RtMidiError &error) {
			gu_log("[KM] unable to open MIDI in port %d: %s\n", port, error.getMessage().c_str());
			status = false;
			return 0;
		}
	}
	else
		return 2;
}

int closeInPort(RtMidiIn *device)
{
	try {
		device->closePort();
		device->cancelCallback();
		gu_log("[KM] MIDI in closed\n");
		return 1;
	}
	catch (RtMidiError &error) {
		gu_log("[KM] unable to close MIDI in: %s\n", error.getMessage().c_str());
	}
	return 0;
}

/* -------------------------------------------------------------------------- */


bool hasAPI(int API)
{
	vector<RtMidi::Api> APIs;
	RtMidi::getCompiledApi(APIs);
	for (unsigned i=0; i<APIs.size(); i++)
		if (APIs.at(i) == API)
			return true;
	return false;
}

/* -------------------------------------------------------------------------- */


void send(uint32_t data)
{
	if (!status)
		return;

	vector<unsigned char> msg(1, getB1(data));
	msg.push_back(getB2(data));
	msg.push_back(getB3(data));

	midiOut->sendMessage(&msg);
	gu_log("[KM] send msg=0x%X (%X %X %X)\n", data, msg[0], msg[1], msg[2]);
}


/* -------------------------------------------------------------------------- */


void send(int b1, int b2, int b3)
{
	if (!status)
		return;

	vector<unsigned char> msg(1, b1);

	if (b2 != -1)
		msg.push_back(b2);
	if (b3 != -1)
		msg.push_back(b3);

	midiOut->sendMessage(&msg);
	//gu_log("[KM] send msg=(%X %X %X)\n", b1, b2, b3);
}


/* -------------------------------------------------------------------------- */


unsigned countInPorts()
{
	return numInPorts;
}


unsigned countOutPorts()
{
	return numOutPorts;
}


/* -------------------------------------------------------------------------- */


int getB1(uint32_t iValue) { return (iValue >> 24) & 0xFF; }
int getB2(uint32_t iValue) { return (iValue >> 16) & 0xFF; }
int getB3(uint32_t iValue) { return (iValue >> 8)  & 0xFF; }


uint32_t getIValue(int b1, int b2, int b3)
{
	return (b1 << 24) | (b2 << 16) | (b3 << 8) | (0x00);
}


/* -------------------------------------------------------------------------- */


uint32_t setChannel(uint32_t iValue, int channel)
{
	uint32_t chanMask = 0xF << 24;
	return (iValue & (~chanMask)) | (channel << 24);
}


/* -------------------------------------------------------------------------- */


bool getStatus()
{
	return status;
}

}}}; // giada::m::kernelMidi::
