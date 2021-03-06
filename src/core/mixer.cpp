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


#include <cassert>
#include <cstring>
#include "../deps/rtaudio-mod/RtAudio.h"
#include "../utils/log.h"
#include "metronome.h"
#include "wave.h"
#include "kernelAudio.h"
#include "recorder.h"
#include "pluginHost.h"
#include "conf.h"
#include "mixerHandler.h"
#include "clock.h"
#include "const.h"
#include "channel.h"
#include "sampleChannel.h"
#include "inputChannel.h"
#include "columnChannel.h"
#include "midiChannel.h"
#include "audioBuffer.h"
#include "mixer.h"

namespace giada {
namespace m {
namespace mixer
{
namespace
{

/* -------------------------------------------------------------------------- */

/* feed
 on InputChannel::process method. */

void routeAudio(AudioBuffer& out, AudioBuffer& in, unsigned bufferSize)
{
	// Feed InputChannels that are currently being used on a recording or being
	// monitored with the input buffer.
	// InputChannel copies de-interleaved buffer[inputIndex] to it's own virtual
	// channel and process throught VSTs.
	// If the InputChannel is monitoring, writes the processed output to outBuf.
	// Also feeds the ColumnChannel it's routed to with the processed output.
	if (kernelAudio::isInputEnabled()) {
		for (unsigned i=0; i<inputChannels.size(); i++) {
			inputChannels[i]->process(out, in);
		}
	}

	// Process ColumnChannels
	// (Input is ignored)
	//
	for (unsigned i=0; i<columnChannels.size(); i++) {
		columnChannels[i]->process(out, in);
	}
}


/* -------------------------------------------------------------------------- */

/* clearAllBuffers
Cleans up every buffer, both in Mixer and in channels. */

void clearAllBuffers(AudioBuffer& outBuf)
{
	outBuf.clear();

	pthread_mutex_lock(&mutex_chans);
	for (InputChannel* ich : inputChannels)
		ich->clearBuffers();
	for (ColumnChannel* cch : columnChannels) {
		cch->clearBuffers();
		for (ResourceChannel* rch : (*cch)) {
			rch->clearBuffers();
		}
	}
	pthread_mutex_unlock(&mutex_chans);
}


/* -------------------------------------------------------------------------- */

/* readActions
Reads all recorded actions. */

void readActions(unsigned frame)
{
	pthread_mutex_lock(&mutex_recs);
	for (unsigned i=0; i<recorder::frames.size(); i++) {
		if (recorder::frames.at(i) != clock::getCurrentFrame())
			continue;
		for (recorder::action* action : recorder::global.at(i)) {
			Channel* ch = mh::getChannelByIndex(action->chan);
			ch->parseAction(action, frame, clock::getCurrentFrame(), clock::isRunning());
		}
		break;
	}
	pthread_mutex_unlock(&mutex_recs);
}


/* -------------------------------------------------------------------------- */

/* doQuantize
Computes quantization on 'rewind' button and all channels. */

void doQuantize(unsigned frame)
{
	/* Nothing to do if quantizer disabled or a quanto has not passed yet. */

	if (clock::getQuantize() == 0 || !clock::quantoHasPassed())
		return;

	if (rewindWait) {
		rewindWait = false;
		rewind();
	}

	pthread_mutex_lock(&mutex_chans);
	for (unsigned i=0; i<columnChannels.size(); i++) {
		ColumnChannel* cch = columnChannels.at(i);
		for (unsigned j=0; j<cch->getResourceCount(); j++)
			cch->getResource(j)->quantize(j, frame, clock::getCurrentFrame());
	}
	pthread_mutex_unlock(&mutex_chans);
}

/* -------------------------------------------------------------------------- */

/* limitOutput
Applies a very dumb hard limiter. */

void limitOutput(AudioBuffer& outBuf, unsigned frame)
{
	for (int i=0; i<outBuf.countChannels(); i++)
		if      (outBuf[frame][i] > 1.0f)
			outBuf[frame][i] = 1.0f;
		else if (outBuf[frame][i] < -1.0f)
			outBuf[frame][i] = -1.0f;
}


/* -------------------------------------------------------------------------- */

/* finalizeOutput
Last touches after the output has been rendered: apply output volume. */

void finalizeOutput(AudioBuffer& outBuf, unsigned frame)
{
	for (int i=0; i<outBuf.countChannels(); i++)
		outBuf[frame][i] *= outVol;
}


/* -------------------------------------------------------------------------- */

/* test*
Checks if the sequencer has reached a specific point (bar, first beat or
last frame). */

void testBar(unsigned frame)
{
	if (!clock::isOnBar())
		return;

	if (metronome::on)
		metronome::tickPlay = true;

	pthread_mutex_lock(&mutex_chans);
	for (unsigned i=0; i<columnChannels.size(); i++) {
		ColumnChannel* cch = columnChannels.at(i);
		for (unsigned j=0; j<cch->getResourceCount(); j++)
			cch->getResource(j)->onBar(frame);
	}
	pthread_mutex_unlock(&mutex_chans);
}


/* -------------------------------------------------------------------------- */


void testFirstBeat(unsigned frame)
{
	if (!clock::isOnFirstBeat())
		return;
	pthread_mutex_lock(&mutex_chans);
	for (unsigned i=0; i<columnChannels.size(); i++) {
		ColumnChannel* cch = columnChannels.at(i);
		for (unsigned j=0; j<cch->getResourceCount(); j++)
			cch->getResource(j)->onZero(frame, conf::recsStopOnChanHalt);
	}
	pthread_mutex_unlock(&mutex_chans);
}


/* -------------------------------------------------------------------------- */


void testLastBeat()
{
	if (clock::isOnBeat())
		if (metronome::on && !metronome::tickPlay) {
			metronome::tockPlay = true;
		}
}

}; // {anonymous}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


std::vector<InputChannel*> inputChannels;
std::vector<ColumnChannel*> columnChannels;

bool   recording    = false;
bool   ready        = true;
float  outVol       = G_DEFAULT_OUT_VOL;
float  inVol        = G_DEFAULT_IN_VOL;
float  peakIn      = 0.0f;
float  peakOut      = 0.0f;
int    waitRec      = 0;
bool   rewindWait   = false;
bool   hasSolos     = false;

pthread_mutex_t mutex_recs;
pthread_mutex_t mutex_chans;
pthread_mutex_t mutex_plugins;


/* -------------------------------------------------------------------------- */


void init(int framesInSeq, int framesInBuffer)
{
	pthread_mutex_init(&mutex_recs, nullptr);
	pthread_mutex_init(&mutex_chans, nullptr);
	pthread_mutex_init(&mutex_plugins, nullptr);
	rewind();
}

/* -------------------------------------------------------------------------- */


int masterPlay(void* outBuf, void* inBuf, unsigned bufferSize,
	double streamTime, RtAudioStreamStatus status, void* userData)
{
	if (!ready)
		return 0;

#ifdef __linux__
	clock::recvJackSync();
#endif

	AudioBuffer out, in;
	out.setData((float*) outBuf, bufferSize, G_OUT_CHANS);
	if (kernelAudio::isInputEnabled())
		in.setData((float*) inBuf, bufferSize, conf::channelsIn);

	peakOut = 0.0f;  // reset peak calculator
	peakIn  = 0.0f;  // reset peak calculator

	clearAllBuffers(out);

	for (unsigned j=0; j<bufferSize; j++) {
		if (clock::isRunning()) {
			doQuantize(j);
			testBar(j);
			testFirstBeat(j);
			readActions(j);
			clock::incrCurrentFrame();
			testLastBeat();  // this test must be the last one
			clock::sendMIDIsync();
		}
	}

	// inBuf -> Input Channels -> Column Channels ->
	// -> Resource Channels -> Column Channels -> _outBuf
	routeAudio(out, in, bufferSize);

	/* post processing */

	/* Post processing. */
	for (unsigned j=0; j<bufferSize; j++) {
		finalizeOutput(out, j);
		if (conf::limitOutput)
			limitOutput(out, j);
		metronome::render(out, j);
	}

	/* Unset data in buffers. If you don't do this, buffers go out of scope and
	destroy memory allocated by RtAudio ---> havoc. */
	out.setData(nullptr, 0, 0);
	in.setData(nullptr, 0, 0);

	return 0;
}


/* -------------------------------------------------------------------------- */


void close()
{
	clock::stop();

	while (inputChannels.size() > 0)
		mh::deleteInputChannel(inputChannels.at(0));

	while (columnChannels.size() > 0)
		mh::deleteColumnChannel(columnChannels.at(0));
}


/* -------------------------------------------------------------------------- */


bool isSilent()
{
	for (unsigned i=0; i<columnChannels.size(); i++)
		if (columnChannels.at(i)->isSilent())
			return false;
	return true;
}


/* -------------------------------------------------------------------------- */


void rewind()
{
	clock::rewind();
	if (clock::isRunning())
		for (unsigned i=0; i<columnChannels.size(); i++) {
		ColumnChannel* cch = columnChannels.at(i);
		for (unsigned j=0; j<cch->getResourceCount(); j++)
			cch->getResource(j)->rewind();
	}
}


/* -------------------------------------------------------------------------- */


}}}; // giada::m::mixer::
