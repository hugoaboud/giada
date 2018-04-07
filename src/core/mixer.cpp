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
#define TICKSIZE 38


float tock[TICKSIZE] = {
	0.059033,  0.117240,  0.173807,  0.227943,  0.278890,  0.325936,
	0.368423,  0.405755,  0.437413,  0.462951,  0.482013,  0.494333,
	0.499738,  0.498153,  0.489598,  0.474195,  0.452159,  0.423798,
	0.389509,  0.349771,  0.289883,  0.230617,  0.173194,  0.118739,
	0.068260,  0.022631, -0.017423, -0.051339,	-0.078721, -0.099345,
 -0.113163, -0.120295, -0.121028, -0.115804, -0.105209, -0.089954,
 -0.070862, -0.048844
};


float tick[TICKSIZE] = {
	0.175860,  0.341914,  0.488904,  0.608633,  0.694426,  0.741500,
	0.747229,  0.711293,	0.635697,  0.524656,  0.384362,  0.222636,
	0.048496, -0.128348, -0.298035, -0.451105, -0.579021, -0.674653,
 -0.732667, -0.749830, -0.688924, -0.594091, -0.474481, -0.340160,
 -0.201360, -0.067752,  0.052194,  0.151746,  0.226280,  0.273493,
	0.293425,  0.288307,  0.262252,  0.220811,  0.170435,  0.117887,
	0.069639,  0.031320
};

int  tickTracker, tockTracker = 0;
bool tickPlay, tockPlay = false; // 1 = play, 0 = stop

/* -------------------------------------------------------------------------- */

/* feed
 on InputChannel::process method. */

void routeAudio(AudioBuffer& out, AudioBuffer& in, unsigned bufferSize)
{
	if (!kernelAudio::isInputEnabled())
		return;

	// Feeds InputChannels that are currently being used on a recording or being
	// monitored with the input buffer.
	// InputChannel copies de-interleaved buffer[inputIndex] to it's own virtual
	// channel and process throught VSTs.
	// If the InputChannel is monitoring, writes the processed output to outBuf.
	// Also feeds the ColumnChannel it's routed to with the processed output.
	for (unsigned i=0; i<inputChannels.size(); i++) {
		inputChannels[i]->process(out, in);
	}

	// Process ColumnChannels
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
	for (InputChannel* ichannel : inputChannels)
		ichannel->clearBuffers();
	for (ColumnChannel* cchannel : columnChannels)
		cchannel->clearBuffers();
	for (Channel* channel : channels)
		channel->clearBuffers();
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

/* renderMetronome
Generates metronome when needed and pastes it to the output buffer. */

void renderMetronome(AudioBuffer& outBuf, unsigned frame)
{
	if (tockPlay) {
		for (int i=0; i<outBuf.countChannels(); i++)
			outBuf[frame][i] += tock[tockTracker];
		tockTracker++;
		if (tockTracker >= TICKSIZE-1) {
			tockPlay    = false;
			tockTracker = 0;
		}
	}
	if (tickPlay) {
		for (int i=0; i<outBuf.countChannels(); i++)
			outBuf[frame][i] += tick[tickTracker];
		tickTracker++;
		if (tickTracker >= TICKSIZE-1) {
			tickPlay    = false;
			tickTracker = 0;
		}
	}
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

	if (metronome)
		tickPlay = true;

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
		if (metronome && !tickPlay)
			tockPlay = true;
}

}; // {anonymous}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


std::vector<InputChannel*> inputChannels;
std::vector<ColumnChannel*> columnChannels;
std::vector<Channel*> channels;

bool   recording    = false;
bool   ready        = true;
float  outVol       = G_DEFAULT_OUT_VOL;
float  inVol        = G_DEFAULT_IN_VOL;
float  peakIn      = 0.0f;
float  peakOut      = 0.0f;
bool	 metronome    = false;
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
	out.setData((float*) outBuf, bufferSize, G_MAX_IO_CHANS);
	if (kernelAudio::isInputEnabled())
		in.setData((float*) inBuf, bufferSize, G_MAX_IO_CHANS);

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
		renderMetronome(out, j);
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
