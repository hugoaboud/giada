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
#include "midiChannel.h"
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


/* -------------------------------------------------------------------------- */

/* inputChannels
Records from line in. */

void toInputChannels(float* outBuf, float* inBuf, unsigned bufferSize)
{
	if (!kernelAudio::isInputEnabled())
		return;

	if (waitRec < conf::delayComp) {
		waitRec += conf::channelsIn;
		return;
	}

	// For each InputChannel, if it's recording or monitoring, process
	// de-interleaved buffer for the patched input. This writes the contents
	// of the buffer to the virtual channel, process throught the VSTs and writes
	// to the output buffer.
	for (unsigned i=0; i<inputChannels.size(); i++) {
		inputChannels[i]->process(outBuf, inBuf);
	}

	/*if (!mh::hasArmedSampleChannels() || !kernelAudio::isInputEnabled() || !recording)
		return;*/

	/* Delay comp: wait until waitRec reaches delayComp. WaitRec
	 * returns to 0 in mixerHandler, as soon as the recording ends*/
	/*
	
	vChanInput[inputTracker]   += inBuf[frame]   * inVol;
	vChanInput[inputTracker+1] += inBuf[frame+1] * inVol;
	inputTracker += 2;
	if (inputTracker >= clock::getTotalFrames())
		inputTracker = 0;*/
}

/* -------------------------------------------------------------------------- */

/* clearAllBuffers
Cleans up every buffer, both in Mixer and in channels. */

void clearAllBuffers(float* outBuf, unsigned bufferSize)
{
	memset(outBuf, 0, sizeof(float) * bufferSize * 2);         // out

	pthread_mutex_lock(&mutex_chans);
	for (InputChannel* ichannel : inputChannels)
		ichannel->clear();
	for (Channel* channel : channels)
		channel->clear();
	pthread_mutex_unlock(&mutex_chans);
}


/* -------------------------------------------------------------------------- */

/* readActions
Reads all recorded actions. */

void readActions(unsigned frame)
{
	pthread_mutex_lock(&mutex_recs);
	for (unsigned i=0; i<recorder::frames.size(); i++) {
		if (recorder::frames.at(i) == clock::getCurrentFrame()) {
			for (unsigned j=0; j<recorder::global.at(i).size(); j++) {
				int index   = recorder::global.at(i).at(j)->chan;
				Channel *ch = mh::getChannelByIndex(index);
				ch->parseAction(recorder::global.at(i).at(j), frame,
					clock::getCurrentFrame(), clock::getQuantize(), clock::isRunning());
			}
			break;
		}
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
	for (unsigned i=0; i<channels.size(); i++)
		channels.at(i)->quantize(i, frame);
	pthread_mutex_unlock(&mutex_chans);
}


/* -------------------------------------------------------------------------- */

/* renderMetronome
Generates metronome when needed and pastes it to the output buffer. */

void renderMetronome(float* outBuf, unsigned frame)
{
	if (tockPlay) {
		outBuf[frame]   += tock[tockTracker];
		outBuf[frame+1] += tock[tockTracker];
		tockTracker++;
		if (tockTracker >= TICKSIZE-1) {
			tockPlay    = false;
			tockTracker = 0;
		}
	}
	if (tickPlay) {
		outBuf[frame]   += tick[tickTracker];
		outBuf[frame+1] += tick[tickTracker];
		tickTracker++;
		if (tickTracker >= TICKSIZE-1) {
			tickPlay    = false;
			tickTracker = 0;
		}
	}
}


/* -------------------------------------------------------------------------- */

/* renderIO
Final processing stage. Take each channel and process it (i.e. copy its
content to the output buffer). Process plugins too, if any. */

void renderIO(float* outBuf, float* inBuf)
{
	pthread_mutex_lock(&mutex_chans);
	for (Channel* channel : channels) {
		channel->process(outBuf, inBuf);
		channel->preview(outBuf);
	}
	pthread_mutex_unlock(&mutex_chans);
}


/* -------------------------------------------------------------------------- */

/* limitOutput
Applies a very dumb hard limiter. */

void limitOutput(float* outBuf, unsigned frame)
{
	if (outBuf[frame] > 1.0f)
		outBuf[frame] = 1.0f;
	else
	if (outBuf[frame] < -1.0f)
		outBuf[frame] = -1.0f;

	if (outBuf[frame+1] > 1.0f)
		outBuf[frame+1] = 1.0f;
	else
	if (outBuf[frame+1] < -1.0f)
		outBuf[frame+1] = -1.0f;
}


/* -------------------------------------------------------------------------- */

/* computePeak */

void computePeak(float* outBuf, unsigned frame)
{
	/* TODO it takes into account only left channel so far! */
	if (outBuf[frame] > peakOut)
		peakOut = outBuf[frame];

	// TODO: calculate input peak (highets InputChannels peak)
}


/* -------------------------------------------------------------------------- */

/* finalizeOutput
Last touches after the output has been rendered: apply output volume. */

void finalizeOutput(float* outBuf, unsigned frame)
{
	outBuf[frame]   *= outVol;
	outBuf[frame+1] *= outVol;
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
	for (unsigned k=0; k<channels.size(); k++)
		channels.at(k)->onBar(frame);
	pthread_mutex_unlock(&mutex_chans);
}


/* -------------------------------------------------------------------------- */


void testFirstBeat(unsigned frame)
{
	if (!clock::isOnFirstBeat())
		return;
	pthread_mutex_lock(&mutex_chans);
	for (unsigned k=0; k<channels.size(); k++)
		channels.at(k)->onZero(frame, conf::recsStopOnChanHalt);
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
std::vector<Channel*> channels;

bool   recording    = false;   // is recording something?
bool   ready        = true;
float  outVol       = G_DEFAULT_OUT_VOL;
float  inVol        = G_DEFAULT_IN_VOL;
float  peakIn      = 0.0f;
float  peakOut      = 0.0f;
bool	 metronome    = false;
int    waitRec      = 0;       // delayComp guard
bool   docross      = false;	 // crossfade guard
bool   rewindWait   = false;	 // rewind guard, if quantized

int  tickTracker, tockTracker = 0;
bool tickPlay, tockPlay = false; // 1 = play, 0 = stop

/* inputTracker
 * position of the sample in the input side (recording) */

int inputTracker = 0;

pthread_mutex_t mutex_recs;
pthread_mutex_t mutex_chans;
pthread_mutex_t mutex_plugins;


/* -------------------------------------------------------------------------- */


void init(int framesInSeq, int audioBufferSize)
{
	/* Allocate virtual input channels. vChanInput has variable size: it depends
	on how many frames there are in sequencer. */

	allocVirtualInput(framesInSeq);

	pthread_mutex_init(&mutex_recs, nullptr);
	pthread_mutex_init(&mutex_chans, nullptr);
	pthread_mutex_init(&mutex_plugins, nullptr);

	rewind();
}


/* -------------------------------------------------------------------------- */


void allocVirtualInput(int frames)
{
	/*if (vChanInput.size() > 0) {
		for (int i = 0; i < vChanInput.size(); i++) delete[] vChanInput[i];
	}
	// TODO: this is probably gonna break on opaque-channels merge
	for (int c = 0; c < conf::channelsIn; c++) vChanInput[c] = new (std::nothrow) float[frames];
	if (!vChanInput)
		gu_log("[Mixer::allocVirtualInput] vChanInput realloc error!\n");	
	gu_log("[Mixer::allocVirtualInput] vChanInput ready, %d frames\n", frames);	*/
}


/* -------------------------------------------------------------------------- */


int masterPlay(void* _outBuf, void* _inBuf, unsigned bufferSize,
	double streamTime, RtAudioStreamStatus status, void* userData)
{
	if (!ready)
		return 0;

#ifdef __linux__
	clock::recvJackSync();
#endif

	float* outBuf = (float*) _outBuf;
	float* inBuf  = kernelAudio::isInputEnabled() ? (float*) _inBuf : nullptr;
	peakOut       = 0.0f;

	clearAllBuffers(outBuf, bufferSize);

	// Feeds InputChannels with inBuf and write processed output
	// of monitoring channels to outBuf.
	toInputChannels(outBuf, inBuf, bufferSize);

	for (unsigned j=0; j<bufferSize*conf::channelsIn; j+=conf::channelsIn) {
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

	renderIO(outBuf, inBuf);

	/* post processing */

	for (unsigned j=0; j<bufferSize; j+=2) {
		finalizeOutput(outBuf, j);
		if (conf::limitOutput)
			limitOutput(outBuf, j);
		computePeak(outBuf, j);
		renderMetronome(outBuf, j);
	}

	return 0;
}


/* -------------------------------------------------------------------------- */


int close()
{
	clock::stop();
	while (channels.size() > 0)
		mh::deleteChannel(channels.at(0));

	/*
	if (vChanInput.size() > 0) {
		for (int c = 0; c < vChanInput.size(); c++) 
			{
				delete[] vChanInput[c];
				vChanInput = nullptr;
			}
	}
	*/

	return 1;
}


/* -------------------------------------------------------------------------- */


bool isSilent()
{
	for (unsigned i=0; i<channels.size(); i++)
		if (channels.at(i)->status == STATUS_PLAY)
			return false;
	return true;
}


/* -------------------------------------------------------------------------- */


void rewind()
{
	clock::rewind();
	if (clock::isRunning())
		for (Channel* ch : channels)
			ch->rewind();
}


/* -------------------------------------------------------------------------- */


void mergeVirtualInput()
{
	/*assert(vChanInput != nullptr);

	for (Channel* ch : channels) {
		if (ch->type == CHANNEL_MIDI)
			continue;
		SampleChannel* sch = static_cast<SampleChannel*>(ch);
		if (sch->isArmed())
			memcpy(sch->wave->getData(), vChanInput, clock::getTotalFrames() * sizeof(float));
	}
	memset(vChanInput, 0, clock::getTotalFrames() * sizeof(float)); // clear vchan*/
}


}}}; // giada::m::mixer::
