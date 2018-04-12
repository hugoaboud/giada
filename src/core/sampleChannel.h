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


#ifndef G_SAMPLE_CHANNEL_H
#define G_SAMPLE_CHANNEL_H


#include <functional>
#include <samplerate.h>
#include "resourceChannel.h"


class Patch;
class Wave;


class SampleChannel : public ResourceChannel
{
private:

	/* fillChan
	Fills 'dest' buffer at point 'offset' with wave data taken from 'start'. If
	rewind=false don't rewind internal tracker. Returns new sample position,
	in frames. It resamples data if pitch != 1.0f. */

	int fillChan(giada::m::AudioBuffer& dest, int start, int offset, bool rewind=true);

	/* calcFadeoutStep
	How many frames are left before the end of the sample? Is there enough room
	for a complete fadeout? Should we shorten it? */

	void calcFadeoutStep();

	/* calcVolumeEnv
	Computes any changes in volume done via envelope tool. */

	void calcVolumeEnv(int frame);

	/* reset
	Rewinds tracker to the beginning of the sample. */

	void reset(int frame);

	/* fade methods
	Prepare channel for fade, mixer will take care of the process during master
	play. */

	void setFadeIn(bool internal);
	void setFadeOut(int actionPostFadeout);
	void setXFade(int frame);

	/* rsmp_state, rsmp_data
	Structs from libsamplerate. */

	SRC_STATE* rsmp_state;
	SRC_DATA   rsmp_data;

	/* pChan, vChanPreview
	Extra virtual channel for processing resampled data and for audio preview. */

	giada::m::AudioBuffer pChan;
	giada::m::AudioBuffer vChanPreview;

	/* inputTracker
	Sample position while recording. */

	int inputTracker;

	/* waitRec
	Delay comp: wait until waitRec reaches conf::delayComp. WaitRec returns to 0
	as soon as the recording ends. */

	int waitRec;

	/* frameRewind
	Exact frame in which a rewind occurs. */

	int frameRewind;

	/* begin, end
	Begin/end point to read wave data from/to. */

	int begin;
	int end;

	float pitch;

	bool  fadeinOn;
	float fadeinVol;
	bool  fadeoutOn;
	float fadeoutVol;      // fadeout volume
	int   fadeoutTracker;  // tracker fadeout, xfade only
	float fadeoutStep;     // fadeout decrease
  int   fadeoutType;     // xfade or fadeout
  int		fadeoutEnd;      // what to do when fadeout ends

public:

	SampleChannel(int bufferSize);
	~SampleChannel();

	/* [Channel] inheritance */
	bool isNodeAlive() override;
	void copy(const Channel* src, pthread_mutex_t* pluginMutex) override;
	void readPatch(const std::string& basePath, int i) override;
	void writePatch(int i, bool isProject) override;
	bool allocBuffers() override;
	void clearBuffers() override;
	void process(giada::m::AudioBuffer& out, giada::m::AudioBuffer& in) override;
	void setMute(bool internal) override;
	void unsetMute(bool internal) override;
	void parseAction(giada::m::recorder::action* a, int localFrame, int globalFrame, bool mixerIsRunning) override;

	/* [ResourceChannel] inheritance */
	void preview(giada::m::AudioBuffer& out) override;
	void start(int frame, bool doQuantize, bool mixerIsRunning, bool forceStart, bool isUserGenerated) override;
	void stop() override;
	void rec(int frame, bool doQuantize, bool mixerIsRunning, bool forceStart, bool isUserGenerated) override;
	void recStart() override;
	void recStop() override;
	void kill(int frame) override;
	void empty() override;
	void stopBySeq(bool chansStopOnSeqHalt) override;
	void quantize(int index, int localFrame, int globalFrame) override;
	void onZero(int frame, bool recsStopOnChanHalt) override;
	void onBar(int frame) override;
	void rewind() override;
	bool canInputRec() override;
	bool startInputRec() override;

	int   getBegin() const;
	int   getEnd() const;
	float getPitch() const;

	/* pushWave
	Adds a new wave to this channel. */

	void pushWave(Wave* w);

	/* getPosition
	Returns the position of an active sample. If EMPTY o MISSING returns -1. */

	int getPosition();

	/* sum
	Adds sample frames to virtual channel. Frame = processed frame in Mixer. */

	void sum(int frame);

	void setPitch(float v);
	void setBegin(int f);
	void setEnd(int f);


	/* hardStop
	Stops the channel immediately, no further checks. */

	void hardStop(int frame);

	/* onPreviewEnd
	A callback fired when audio preview ends. */

	std::function<void()> onPreviewEnd;

	Wave* wave;
	int   tracker;         // chan position
	int   trackerPreview;  // chan position for audio preview
	int   shift;
	int   mode;            // mode: see const.h
	bool  qWait;           // quantizer wait

	/* midi stuff */

  bool     midiInVeloAsVol;
  uint32_t midiInReadActions;
  uint32_t midiInPitch;

	/* const - what to do when a fadeout ends */

	enum {
		DO_STOP   = 0x01,
		DO_MUTE   = 0x02,
		DO_MUTE_I = 0x04
	};

	/*  const - fade types */

	enum {
		FADEOUT = 0x01,
		XFADE   = 0x02
	};
};

#endif
