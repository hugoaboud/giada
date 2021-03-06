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


#ifndef G_MIDI_CHANNEL_H
#define G_MIDI_CHANNEL_H


#ifdef WITH_VST
	#include "../deps/juce-config.h"
#endif
#include "resourceChannel.h"


class MidiMapConf;
class Patch;


class MidiChannel : public ResourceChannel
{

public:

	MidiChannel(int bufferSize);
	~MidiChannel();

	/* [Channel] inheritance */
	void copy(const Channel* src, pthread_mutex_t* pluginMutex) override;
	void readPatch(const std::string& basePath, int i) override;
	void writePatch(bool isProject) override;
	void clearBuffers() override;
	void process(giada::m::AudioBuffer& out, giada::m::AudioBuffer& in) override;
	void setMute(bool internal) override;
	void unsetMute(bool internal) override;
	void parseAction(giada::m::recorder::action* a, int localFrame, int globalFrame, bool mixerIsRunning) override;
	void receiveMidi(const giada::m::MidiEvent& midiEvent) override;

	/* [ResourceChannel] inheritance */
	void preview(giada::m::AudioBuffer& out) override;
	void start(int frame, bool doQuantize, bool mixerIsRunning, bool forceStart, bool isUserGenerated) override;
	void stop() override;
	void rec(int frame, bool doQuantize, bool mixerIsRunning, bool forceStart, bool isUserGenerated) override;
	void recStart(bool force=false) override;
	void recStop(bool force=false) override;
	void kill(int frame) override;
	void empty() override;
	void stopBySeq(bool chansStopOnSeqHalt) override;
	void quantize(int index, int localFrame, int globalFrame) override;
	void onZero(int frame, bool recsStopOnChanHalt) override;
	void onBar(int frame) override;
	void rewind() override;
	bool canInputRec() override;
	bool startInputRec() override;
	void stopInputRec() override;

	void setBegin(int f) override;
	void setEnd(int f) override;
	int getPosition() override;

	/* sendMidi
	 * send Midi event to the outside world. */

	void sendMidi(giada::m::recorder::action* a, int localFrame);
	void sendMidi(uint32_t data);

	bool    midiOut;           // enable midi output
	uint8_t midiOutChan;       // midi output channel
};


#endif
