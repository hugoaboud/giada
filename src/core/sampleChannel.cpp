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


#include <cmath>
#include <cstring>
#include <cassert>
#include "sampleChannel.h"
#include "../utils/log.h"
#include "../utils/fs.h"
#include "../utils/string.h"
#include "../gui/elems/mainWindow/keyboard/sampleChannel.h"
#include "patch.h"
#include "channelManager.h"
#include "const.h"
#include "conf.h"
#include "clock.h"
#include "mixer.h"
#include "wave.h"
#include "pluginHost.h"
#include "waveFx.h"
#include "waveManager.h"
#include "mixerHandler.h"
#include "kernelMidi.h"
#include "kernelAudio.h"
#include "resourceChannel.h"



using std::string;
using namespace giada::m;


SampleChannel::SampleChannel(int bufferSize)
	: ResourceChannel  (G_CHANNEL_SAMPLE, STATUS_EMPTY, bufferSize),
		rsmp_state       (nullptr),
		inputTracker		 (0),
		frameRewind      (-1),
		pitch            (G_DEFAULT_PITCH),
		fadeinOn         (false),
		fadeinVol        (1.0f),
		fadeoutOn        (false),
		fadeoutVol       (1.0f),
		fadeoutTracker   (0),
		fadeoutStep      (G_DEFAULT_FADEOUT_STEP),
		wave             (nullptr),
		tracker          (0),
		trackerPreview   (0),
		shift            (0),
		qWait	           (false),
		qRecWait	           (false),
		midiInReadActions(0x0),
		midiInPitch      (0x0)
{
}


/* -------------------------------------------------------------------------- */


SampleChannel::~SampleChannel()
{
	if (wave != nullptr)
		delete wave;
	if (rsmp_state != nullptr)
		src_delete(rsmp_state);
}


/* -------------------------------------------------------------------------- */


bool SampleChannel::allocBuffers()
{
	if (!Channel::allocBuffers())
		return false;

	rsmp_state = src_new(SRC_LINEAR, mono?1:2, nullptr);
	if (rsmp_state == nullptr) {
		gu_log("[SampleChannel::allocBuffers] unable to alloc memory for SRC_STATE!\n");
		return false;
	}

	if (!pChan.alloc(bufferSize, mono?1:2)) {
		gu_log("[SampleChannel::allocBuffers] unable to alloc memory for pChan!\n");
		return false;
	}

	if (!vChanPreview.alloc(bufferSize, mono?1:2)) {
		gu_log("[SampleChannel::allocBuffers] unable to alloc memory for vChanPreview!\n");
		return false;
	}

	return true;
}


/* -------------------------------------------------------------------------- */


void SampleChannel::copy(const Channel* src_, pthread_mutex_t* pluginMutex)
{
	Channel::copy(src_, pluginMutex);
	const SampleChannel* src = static_cast<const SampleChannel*>(src_);
	tracker         = src->tracker;
	begin           = src->begin;
	end             = src->end;
	boost           = src->boost;
	mode            = src->mode;
	recMode         = src->recMode;
	qWait           = src->qWait;
	fadeinOn        = src->fadeinOn;
	fadeinVol       = src->fadeinVol;
	fadeoutOn       = src->fadeoutOn;
	fadeoutVol      = src->fadeoutVol;
	fadeoutTracker  = src->fadeoutTracker;
	fadeoutStep     = src->fadeoutStep;
	fadeoutType     = src->fadeoutType;
	fadeoutEnd      = src->fadeoutEnd;
	setPitch(src->pitch);

	if (src->wave)
		pushWave(new Wave(*src->wave)); // invoke Wave's copy constructor
}


/* -------------------------------------------------------------------------- */


void SampleChannel::clearBuffers()
{
	/** TODO - these clear() may be done only if status PLAY | ENDING (if below),
	 * but it would require extra clearPChan calls when samples stop */

	vChan.clear();
	pChan.clear();
	vChanPreview.clear();
}


/* -------------------------------------------------------------------------- */


void SampleChannel::calcVolumeEnv(int frame)
{
	/* method: check this frame && next frame, then calculate delta */

	recorder::action* a0 = nullptr;
	recorder::action* a1 = nullptr;
	int res;

	/* get this action on frame 'frame'. It's unlikely that the action
	 * is not found. */

	res = recorder::getAction(index, G_ACTION_VOLUME, frame, &a0);
	if (res == 0)
		return;

	/* get the action next to this one.
	 * res == -1: a1 not found, this is the last one. Rewind the search
	 * and use action at frame number 0 (actions[0]).
	 * res == -2 G_ACTION_VOLUME not found. This should never happen */

	res = recorder::getNextAction(index, G_ACTION_VOLUME, frame, &a1);

	if (res == -1)
		res = recorder::getAction(index, G_ACTION_VOLUME, 0, &a1);

	volume_i = a0->fValue;
	volume_d = ((a1->fValue - a0->fValue) / (a1->frame - a0->frame)) * 1.003f;
}


/* -------------------------------------------------------------------------- */


void SampleChannel::hardStop(int frame)
{
	if (frame != 0)
		vChan.clear(frame); // clear data in range [frame, [end]]
	status = STATUS_OFF;
	sendMidiLplay();
	reset(frame);
}


/* -------------------------------------------------------------------------- */


void SampleChannel::onBar(int frame)
{
	///if (mode == LOOP_REPEAT && status == STATUS_PLAY)
	///	//setXFade(frame * 2);
	///	reset(frame * 2);

	if (mode == LOOP_REPEAT) {
		if (status == STATUS_PLAY)
			//setXFade(frame * 2);
			reset(frame);
	}
	else
	if (mode == LOOP_ONCE_BAR) {
		if (status == STATUS_WAIT && !qWait) {
			status  = STATUS_PLAY;
			tracker = fillChan(vChan, tracker, frame);
			sendMidiLplay();
		}
	}
}


/* -------------------------------------------------------------------------- */


void SampleChannel::setBegin(int f)
{
	if (f < 0)
		begin = 0;
	else
	if (f > wave->getSize())
		begin = wave->getSize();
	else
	if (f >= end)
		begin = end - 1;
	else
		begin = f;

	tracker = begin;
	trackerPreview = begin;
}


/* -------------------------------------------------------------------------- */


void SampleChannel::setEnd(int f)
{
	if (f < 0)
		end = begin + wave->getChannels();
	else
	if (f >= wave->getSize())
		end = wave->getSize() - 1;
	else
	if (f <= begin)
		end = begin + 1;
	else
		end = f;
}

/* -------------------------------------------------------------------------- */


void SampleChannel::setPitch(float v)
{
	if (v > G_MAX_PITCH)
		pitch = G_MAX_PITCH;
	else
	if (v < 0.1f)
		pitch = 0.1000f;
	else
		pitch = v;

	rsmp_data.src_ratio = 1/pitch;

	/* if status is off don't slide between frequencies */

	if (status & (STATUS_OFF | STATUS_WAIT))
		src_set_ratio(rsmp_state, 1/pitch);
}


float SampleChannel::getPitch() const { return pitch; }


/* -------------------------------------------------------------------------- */


void SampleChannel::rewind()
{
	/* rewind LOOP_ANY or SINGLE_ANY only if it's in read-record-mode */

	if (wave != nullptr) {
		if ((mode & LOOP_ANY) || (recStatus == REC_READING && (mode & SINGLE_ANY)))
			reset(0);  // rewind is user-generated events, always on frame 0
	}
}


/* -------------------------------------------------------------------------- */


void SampleChannel::parseAction(recorder::action* a, int localFrame, int globalFrame, bool mixerIsRunning)
{
	if (readActions == false)
		return;

	switch (a->type) {
		case G_ACTION_KEYPRESS:
			if (mode & SINGLE_ANY)
				start(localFrame, false, mixerIsRunning, false, false);
			break;
		case G_ACTION_KEYREL:
			if (mode & SINGLE_ANY)
				stop();
			break;
		case G_ACTION_KILL:
			if (mode & SINGLE_ANY)
				kill(localFrame);
			break;
		case G_ACTION_MUTEON:
			setMute(true);   // internal mute
			break;
		case G_ACTION_MUTEOFF:
			unsetMute(true); // internal mute
			break;
		case G_ACTION_VOLUME:
			calcVolumeEnv(globalFrame);
			break;
	}
}


/* -------------------------------------------------------------------------- */


void SampleChannel::sum(int frame)
{
	if (wave == nullptr || !(isPlaying()))
		return;

	bool running = clock::isRunning();
	if (frame != frameRewind) {

		/* volume envelope, only if seq is running */

		if (running) {
			volume_i += volume_d;
			if (volume_i < 0.0f)
				volume_i = 0.0f;
			else
			if (volume_i > 1.0f)
				volume_i = 1.0f;
		}

		/* fadein or fadeout processes. If mute, delete any signal. */

		/** TODO - big issue: fade[in/out]Vol * internal_volume might be a
		 * bad choice: it causes glitches when muting on and off during a
		 * volume envelope. */

		if (mute || mute_i) {
			for (int i=0; i<vChan.countChannels(); i++)
				vChan[frame][i] = 0.0f;
		}
		else
		if (fadeinOn) {
			if (fadeinVol < 1.0f) {
				for (int i=0; i<vChan.countChannels(); i++)
					vChan[frame][i] *= fadeinVol * volume_i;
				fadeinVol += 0.01f;
			}
			else {
				fadeinOn  = false;
				fadeinVol = 0.0f;
			}
		}
		else
		if (fadeoutOn) {
			if (fadeoutVol > 0.0f) { // fadeout ongoing
				if (fadeoutType == XFADE) {
					for (int i=0; i<vChan.countChannels(); i++)
						vChan[frame][i] = pChan[frame][i] * fadeoutVol * volume_i;
				}
				else {
					for (int i=0; i<vChan.countChannels(); i++)
						vChan[frame][i] *= fadeoutVol * volume_i;
				}
				fadeoutVol -= fadeoutStep;
			}
			else {  // fadeout end
				fadeoutOn  = false;
				fadeoutVol = 1.0f;

				/* QWait ends with the end of the xfade */

				if (fadeoutType == XFADE) {
					qWait = false;
				}
				else {
					if (fadeoutEnd == DO_MUTE)
						mute = true;
					else
					if (fadeoutEnd == DO_MUTE_I)
						mute_i = true;
					else             // DO_STOP
						hardStop(frame);
				}
			}
		}
		else {
			for (int i=0; i<vChan.countChannels(); i++)
				vChan[frame][i] *= volume_i;
		}
	}
	else { // at this point the sample has reached the end */

		if (mode & (SINGLE_BASIC | SINGLE_PRESS | SINGLE_RETRIG) ||
			 (mode == SINGLE_ENDLESS && status == STATUS_ENDING)   ||
			 (mode & LOOP_ANY && !running))     // stop loops when the seq is off
		{
			status = STATUS_OFF;
			sendMidiLplay();
		}

		/* LOOP_ONCE or LOOP_ONCE_BAR: if ending (i.e. the user requested their
		 * termination), kill 'em. Let them wait otherwise. But don't put back in
		 * wait mode those already stopped by the conditionals above. */

		if (mode & (LOOP_ONCE | LOOP_ONCE_BAR)) {
			if (status == STATUS_ENDING)
				status = STATUS_OFF;
			else
			if (status != STATUS_OFF)
				status = STATUS_WAIT;
		}

		/* Check for end of samples. SINGLE_ENDLESS runs forever unless it's in
		ENDING mode. */

		reset(frame);
	}
}


/* -------------------------------------------------------------------------- */


void SampleChannel::onZero(int frame, bool recsStopOnChanHalt)
{
	if (wave == nullptr)
	{
		if (recMode & REC_ZERO_ANY && recStatus == REC_WAITING) {
			setReadActions(true, recsStopOnChanHalt);   // rec start
			startInputRec();
		}
		return;
	}

	if (recStatus & REC_ENDING || ((recMode & REC_SINGLE_ANY & REC_ZERO_ANY) && isRecording())) {
		setReadActions(false, recsStopOnChanHalt);  // rec stop
		inputTracker = wave->getSize()-1;
		stopInputRec();
		armed = false;
	}

	if (mode & LOOP_ANY) {

		if (status == STATUS_WAIT) {
			status  = STATUS_PLAY;
			qWait = false;
			sendMidiLplay();
			tracker = fillChan(vChan, tracker, frame);
		}

		/* do a crossfade if the sample is playing. Regular chanReset
		 * instead if it's muted, otherwise a click occurs */

		if (status == STATUS_PLAY) {
			/*
			if (mute || mute_i)
				reset(frame * 2);
			else
				setXFade(frame * 2);
			*/
			reset(frame);
		}
		else
		if (status == STATUS_ENDING)
			hardStop(frame);
	}

	((geSampleChannel*)guiChannel)->update();
}


/* -------------------------------------------------------------------------- */


void SampleChannel::quantize(int index, int localFrame, int globalFrame)
{
	/* skip if LOOP_ANY or not in quantizer-wait mode */

	if ((mode & SINGLE_ANY) && qWait) {

		/* no fadeout if the sample starts for the first time (from a
		 * STATUS_OFF), it would be meaningless. */

		if (status == STATUS_WAIT) {
			status  = STATUS_PLAY;
			sendMidiLplay();
			qWait   = false;
			tracker = fillChan(vChan, tracker, localFrame); /// FIXME: ???
		}
		else
			//setXFade(localFrame * 2);
			reset(localFrame);

		/* this is the moment in which we record the keypress, if the
		 * quantizer is on. SINGLE_PRESS needs overdub */

		if (recorder::canRec(this, clock::isRunning(), mixer::recording)) {
			if (mode == SINGLE_PRESS) {
				recorder::startOverdub(index, G_ACTION_KEYS, globalFrame,
					kernelAudio::getRealBufSize());
	      readActions = false;   // don't read actions while overdubbing
	    }
			else
				recorder::rec(index, G_ACTION_KEYPRESS, globalFrame);
	    hasActions = true;
		}

	}

	if ((recMode & REC_Q_ANY) && qRecWait) {
		if (recStatus & (REC_STOPPED | REC_WAITING)) startInputRec();
		else stopInputRec();
	}
}


/* -------------------------------------------------------------------------- */


int SampleChannel::getPosition()
{
	if (status & ~(STATUS_EMPTY | STATUS_MISSING | STATUS_OFF)) // if is not (...)
		return tracker - begin;
	else
		return -1;
}


/* -------------------------------------------------------------------------- */


void SampleChannel::setMute(bool internal)
{
	if (internal) {

		/* global mute is on? don't waste time with fadeout, just mute it
		 * internally */

		if (mute)
			mute_i = true;
		else {
			if (isPlaying())
				setFadeOut(DO_MUTE_I);
			else
				mute_i = true;
		}
	}
	else {

		/* internal mute is on? don't waste time with fadeout, just mute it
		 * globally */

		if (mute_i)
			mute = true;
		else {

			/* sample in play? fadeout needed. Else, just mute it globally */

			if (isPlaying())
				setFadeOut(DO_MUTE);
			else
				mute = true;
		}
	}

	sendMidiLmute();
}


/* -------------------------------------------------------------------------- */


void SampleChannel::unsetMute(bool internal)
{
	if (internal) {
		if (mute)
			mute_i = false;
		else {
			if (isPlaying())
				setFadeIn(internal);
			else
				mute_i = false;
		}
	}
	else {
		if (mute_i)
			mute = false;
		else {
			if (isPlaying())
				setFadeIn(internal);
			else
				mute = false;
		}
	}

	sendMidiLmute();
}

/* -------------------------------------------------------------------------- */


void SampleChannel::calcFadeoutStep()
{
	if (end - tracker < (1 / G_DEFAULT_FADEOUT_STEP))
		fadeoutStep = ceil((end - tracker) / volume); /// or volume_i ???
	else
		fadeoutStep = G_DEFAULT_FADEOUT_STEP;
}

/* -------------------------------------------------------------------------- */


void SampleChannel::setFadeIn(bool internal)
{
	if (internal) mute_i = false;  // remove mute before fading in
	else          mute   = false;
	fadeinOn  = true;
	fadeinVol = 0.0f;
}


/* -------------------------------------------------------------------------- */


void SampleChannel::setFadeOut(int actionPostFadeout)
{
	calcFadeoutStep();
	fadeoutOn   = true;
	fadeoutVol  = 1.0f;
	fadeoutType = FADEOUT;
	fadeoutEnd	= actionPostFadeout;
}


/* -------------------------------------------------------------------------- */


void SampleChannel::setXFade(int frame)
{
	gu_log("[xFade] frame=%d tracker=%d\n", frame, tracker);

	calcFadeoutStep();
	fadeoutOn      = true;
	fadeoutVol     = 1.0f;
	fadeoutType    = XFADE;
	fadeoutTracker = fillChan(pChan, tracker, 0, false);
	reset(frame);
}


/* -------------------------------------------------------------------------- */


void SampleChannel::reset(int frame)
{
	//fadeoutTracker = tracker;   // store old frame number for xfade
	tracker = begin;
	mute_i  = false;
	qWait   = false;  // Was in qWait mode? Reset occured, no more qWait now.

	/* On reset, if frame > 0 and in play, fill again pChan to create something
	like this:

		|abcdefabcdefab*abcdefabcde|
		[old data-----]*[new data--] */

	if (frame > 0 && status & (STATUS_PLAY | STATUS_ENDING))
		tracker = fillChan(vChan, tracker, frame);
}

/* -------------------------------------------------------------------------- */


void SampleChannel::empty()
{
	status = STATUS_OFF;
	if (wave != nullptr) {
		delete wave;
		wave = nullptr;
	}
  begin   = 0;
  end     = 0;
  tracker = 0;
	status  = STATUS_EMPTY;
  volume  = G_DEFAULT_VOL;
  boost   = G_DEFAULT_BOOST;
	sendMidiLplay();
}


/* -------------------------------------------------------------------------- */


void SampleChannel::pushWave(Wave* w)
{
	sendMidiLplay();     // FIXME - why here?!?!
	wave   = w;
	status = STATUS_OFF;
	begin  = 0;
	end    = wave->getSize() - 1;
	name   = wave->getBasename();
}

/* -------------------------------------------------------------------------- */

void SampleChannel::process(giada::m::AudioBuffer& out, giada::m::AudioBuffer& in)
{
	if (mute) return;
	assert(out.countSamples() == vChan.countSamples());
	assert(in.countSamples()  == vChan.countSamples());

	// playback
	if (isPlaying() && !pre_mute) {
		tracker = fillChan(vChan, tracker, 0);
		if (fadeoutOn && fadeoutType == XFADE) {
			gu_log("[clear] filling pChan fadeoutTracker=%d\n", fadeoutTracker);
			fadeoutTracker = fillChan(pChan, fadeoutTracker, 0);
		}
		for (int i = 0; i < bufferSize; i++)
			sum(i);
	}

	// overdub monitor
	bool overdub = (recMode & REC_OVERDUB_ANY) && isRecording() && !pre_mute;
	if (overdub) {
		if (waitRec >= conf::delayComp) {
			fillChan(vChan, inputTracker, 0);
		}
	}

	// recording
	if (isRecording()) {
		if (waitRec < conf::delayComp) {
			waitRec++;
		}
		else {
			int waveSize = wave->getSize();
			for (int i=0; i<vChan.countFrames(); i++) {
				for (int j=0; j<vChan.countChannels(); j++) {
					(*wave)[inputTracker][j] += in[i][j];   // add, don't overwrite
				}
				inputTracker++;
				if (inputTracker >= waveSize) {
					if (recMode & REC_SINGLE_ANY) {
						stopInputRec();
						break;
					}
					else inputTracker -= waveSize;
				}
			}
		}
	}

	// input monitor
	if (!pre_mute && inputMonitor)
		input(in);

	// processing
	#ifdef WITH_VST
		pluginHost::processStack(vChan, this);
	#endif

	// output
	if (inputMonitor || isPlaying() || overdub)
		output(out);
}


/* -------------------------------------------------------------------------- */


void SampleChannel::preview(giada::m::AudioBuffer& out)
{
	if (previewMode == G_PREVIEW_NONE)
		return;

	/* If the tracker exceedes the end point and preview is looped, split the
	rendering as in SampleChannel::reset(). */

	if (trackerPreview + bufferSize >= end) {
		int offset = end - trackerPreview;
		trackerPreview = fillChan(vChanPreview, trackerPreview, 0, false);
		trackerPreview = begin;
		if (previewMode == G_PREVIEW_LOOP)
			trackerPreview = fillChan(vChanPreview, begin, offset, false);
		else
		if (previewMode == G_PREVIEW_NORMAL) {
			previewMode = G_PREVIEW_NONE;
			if (onPreviewEnd)
				onPreviewEnd();
		}
	}
	else
		trackerPreview = fillChan(vChanPreview, trackerPreview, 0, false);

	for (int i=0; i<out.countFrames(); i++)
		for (int j=0; j<out.countChannels(); j++)
			out[i][j] += vChanPreview[i][j] * volume * calcPanning(j) * boost;
}


/* -------------------------------------------------------------------------- */


void SampleChannel::kill(int frame)
{
	if (wave != nullptr && status != STATUS_OFF) {
		if (mute || mute_i || (status == STATUS_WAIT && mode & LOOP_ANY))
			hardStop(frame);
		else
			setFadeOut(DO_STOP);
	}
}


/* -------------------------------------------------------------------------- */


void SampleChannel::stopBySeq(bool chansStopOnSeqHalt)
{
  /* Loop-mode samples in wait status get stopped right away. */

	if (mode & LOOP_ANY && status == STATUS_WAIT && !qWait) {
		status = STATUS_OFF;
    return;
  }

  /* When to kill samples on StopSeq:
   *  - when chansStopOnSeqHalt == true (run the sample to end otherwise)
   *  - when a channel has recs in play (1)
   *
   * Always kill at frame=0, this is a user-generated event.
   *
   * (1) a channel has recs in play when:
   *  - Recorder has events for that channel
   *  - G_Mixer has at least one sample in play
   *  - Recorder's channel is active (altrimenti può capitare che si stoppino i
   *    sample suonati manualmente in un canale con rec disattivate) */

	if (chansStopOnSeqHalt) {
    if ((mode & LOOP_ANY) || (hasActions && readActions && status == STATUS_PLAY))
      kill(0);
  }
}


/* -------------------------------------------------------------------------- */


void SampleChannel::stop()
{
	if (mode == SINGLE_PRESS && status == STATUS_PLAY) {
		if (mute || mute_i)
			hardStop(0);  /// FIXME - wrong frame value
		else
			setFadeOut(DO_STOP);
	}
	else  // stop a SINGLE_PRESS immediately, if the quantizer is on
	if (mode == SINGLE_PRESS && qWait == true)
		qWait = false;
}


/* -------------------------------------------------------------------------- */


void SampleChannel::readPatch(const string& basePath, int i)
{
	Channel::readPatch("", i);
	channelManager::readPatch(this, basePath, i);
}


/* -------------------------------------------------------------------------- */


bool SampleChannel::canInputRec()
{
	return wave == nullptr && armed;
}

bool SampleChannel::startInputRec()
{
		Wave*  w = nullptr;
		string name = string("TAKE-" + gu_iToString(patch::lastTakeId++)); // Increase lastTakeId

		int result = waveManager::createEmpty(clock::getFramesInLoop(), mono?1:2,
			conf::samplerate, name + ".wav", &w);
		if (result != G_RES_OK)
			return false;

		pushWave(w);

		recStatus = REC_READING;
		qRecWait = false;
		waitRec = 0;
		armed = false;
		inputTracker = 0;
		((geSampleChannel*)guiChannel)->update();
		return true;
}

void SampleChannel::stopInputRec()
{
	recStatus = REC_STOPPED;
	qRecWait = false;
	setEnd(inputTracker);

	// play mode
	if (recMode & REC_PLAY_ANY) {
		status = STATUS_PLAY;
	}
}


/* -------------------------------------------------------------------------- */


void SampleChannel::start(int frame, bool doQuantize, bool mixerIsRunning, bool forceStart, bool isUserGenerated)
{
	// Stop recording if any
	if (recStatus != REC_STOPPED) {
		if (recStatus == REC_WAITING) {
			recStatus = REC_STOPPED;
		}
		else {
			recStop();
			status = STATUS_OFF;
		}
	}

	switch (status)	{
		case STATUS_EMPTY:
		case STATUS_MISSING:
		case STATUS_WRONG:
		{
			return;
		}
		case STATUS_OFF:
		{
			if (mode & LOOP_ANY) {
        if (forceStart) {
          status = STATUS_PLAY;
          tracker = frame;
        }
        else
				    status = STATUS_WAIT;
			}
			else {
				if (mixerIsRunning && doQuantize) {
					qWait = true;
					status = STATUS_WAIT;
				}
				else {
					status = STATUS_PLAY;

					/* Do fillChan only if this is not a user-generated event (i.e. is an
					action read by Mixer). Otherwise clear() will take take of calling
					fillChan on the next cycle. */

					if (!isUserGenerated)
						tracker = fillChan(vChan, tracker, frame);
				}
			}
			sendMidiLplay();
			break;
		}
		case STATUS_PLAY:
		{
			if (mode == SINGLE_BASIC)
				setFadeOut(DO_STOP);
			else
			if (mode == SINGLE_RETRIG) {
				if (mixerIsRunning && doQuantize)
					qWait = true;
				else
					reset(frame);
			}
			else
			if (mode & (LOOP_ANY | SINGLE_ENDLESS)) {
				status = STATUS_ENDING;
				sendMidiLplay();
			}
			break;
		}
		case STATUS_WAIT:
		{
			status = STATUS_OFF;
			qWait = false;
			sendMidiLplay();
			break;
		}
		case STATUS_ENDING:
		{
			status = STATUS_PLAY;
			sendMidiLplay();
			break;
		}
	}
}


void SampleChannel::rec(int frame, bool doQuantize, bool mixerIsRunning, bool forceStart, bool isUserGenerated)
{
	if (status & !(STATUS_EMPTY & STATUS_OFF)) return;

	if (!mixer::recording) {
		armed = !armed;
		guiChannel->update();
		return;
	}

	switch (recStatus) {
		case REC_ENDING: {
			recStatus = REC_READING;
			qRecWait = false;
			break;
		}
		case REC_STOPPED:
		{
			recStart(forceStart);
			break;
		}
		case REC_READING:
		{
			if (recMode & REC_SINGLE_ANY & REC_ZERO_ANY) {
				// TODO: record one more layer
			}
			else recStop(forceStart);
			break;
		}
		case REC_WAITING:
		{
			recStatus = REC_STOPPED;
			qRecWait = false;
			break;
		}
	}
	guiChannel->update();
}

/* -------------------------------------------------------------------------- */

void SampleChannel::recStart(bool force) {
	bool doQuantize = clock::getQuantize() != 0;
	bool running = clock::isRunning();

	if (status == STATUS_EMPTY && recStatus == REC_STOPPED) {

		if (recMode & REC_ZERO_ANY) {
			if (force) {
				startInputRec();
				setReadActions(true, false);
			}
			else recStatus = REC_WAITING;
		}
		else {
			if (running && doQuantize) {
				qRecWait = true;
				recStatus = REC_WAITING;
			}
			else {
				startInputRec();
				setReadActions(true, false);
			}
		}

		guiChannel->update();
	}
}

/* -------------------------------------------------------------------------- */

void SampleChannel::recStop(bool force) {
	bool doQuantize = clock::getQuantize() != 0;
	bool running = clock::isRunning() != 0;

	if (force) {
		stopInputRec();
		setReadActions(true, false);
	}
	else {
		if (recMode & REC_Q_ANY) {
			if (running && doQuantize) {
				qRecWait = true;
				recStatus = REC_ENDING;
			}
			else {
				stopInputRec();
				setReadActions(true, false);
			}
		}
		else {
			recStatus = REC_ENDING;
		}
	}

	guiChannel->update();
}

/* -------------------------------------------------------------------------- */

void SampleChannel::writePatch(bool isProject)
{
	Channel::writePatch(isProject);
	channelManager::writePatch(this, isProject, index);
}


/* -------------------------------------------------------------------------- */


int SampleChannel::fillChan(giada::m::AudioBuffer& dest, int start, int offset, bool rewind)
{
	int position;  // return value: the new position

	if (pitch == 1.0f) {

		/* Fill 'dest' buffer with a chunk of data of size 'buffersize - offset'.
		Case 1: there is enough data in Wave. Case2: not enough data or Wave is
		smaller than the buffer. */

		int chunkSize = bufferSize - offset;
		if (start + chunkSize <= end) {
			position = start + chunkSize;
			if (rewind)
				frameRewind = -1;
		}
		else {
			chunkSize = end - start;
			position  = end;
			if (rewind)
				frameRewind = chunkSize + offset;
		}
		dest.copyData(wave->getFrame(start), chunkSize, offset);
	}
	else {
		rsmp_data.data_in       = wave->getFrame(start);    // source data
		rsmp_data.input_frames  = end - start;              // how many readable bytes
		rsmp_data.data_out      = dest[offset];             // destination (processed data)
		rsmp_data.output_frames = bufferSize - offset;      // how many bytes to process
		rsmp_data.end_of_input  = false;

		src_process(rsmp_state, &rsmp_data);

		position = start + rsmp_data.input_frames_used;     // position goes forward of frames_used (i.e. read from wave)

		if (rewind) {
			int gen = rsmp_data.output_frames_gen;            // frames generated by this call
			if (gen == bufferSize - offset)
				frameRewind = -1;
			else
				frameRewind = gen + offset;
		}
	}
	return position;
}
