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
#include "channel.h"


class Patch;
class Wave;


class SampleChannel : public Channel
{
private:

	/* fillChan
	Fills 'dest' buffer at point 'offset' with wave data taken from 'start'. If 
	rewind=false don't rewind internal tracker. Returns new sample position, 
	in frames. It resamples data if pitch != 1.0f. */

	int fillChan(float* dest, int start, int offset, bool rewind=true);

	/* clearChan
	 * set data to zero from start to bufferSize-1. */

	void clearChan(float* dest, int start);

	/* calcFadeoutStep
	 * how many frames are left before the end of the sample? Is there
	 * enough room for a complete fadeout? Should we shorten it? */

	void calcFadeoutStep();

	/* calcVolumeEnv
	 * compute any changes in volume done via envelope tool */

	void calcVolumeEnv(int frame);

	/* rsmp_state, rsmp_data
	 * structs from libsamplerate */

	SRC_STATE* rsmp_state;
	SRC_DATA   rsmp_data;

	/* pChan
	Extra virtual channel for processing resampled data. */

	float* pChan;

	/* pChan
	Extra virtual channel for audio preview. */

	float* vChanPreview;

	/* frameRewind
	Exact frame in which a rewind occurs. */

	int frameRewind;

	int   begin;
	int   end;
	float boost;
	float pitch;
	int   trackerPreview;  // chan position for audio preview
	int   shift;

	/* onPreviewEnd
	A callback fired when audio preview ends. */

	std::function<void()> onPreviewEnd;

public:

	SampleChannel(int bufferSize, bool inputMonitor);
	~SampleChannel();

	void copy(const Channel* src, pthread_mutex_t* pluginMutex) override;
	void clear() override;
	void input(float* inBuffer) override;
	void process(float* outBuffer, float* inBuffer) override;
	void preview(float* outBuffer) override;
	void start(int frame, bool doQuantize, int quantize, bool mixerIsRunning,
		bool forceStart, bool isUserGenerated) override;
	void kill(int frame) override;
	void empty() override;
	void stopBySeq(bool chansStopOnSeqHalt) override;
	void stop() override;
	void rewind() override;
	void setMute(bool internal) override;
	void unsetMute(bool internal) override;
  int readPatch(const std::string& basePath, int i, pthread_mutex_t* pluginMutex,
    int samplerate, int rsmpQuality) override;
	int writePatch(int i, bool isProject) override;
	void quantize(int index, int localFrame) override;
	void onZero(int frame, bool recsStopOnChanHalt) override;
	void onBar(int frame) override;
	void parseAction(giada::m::recorder::action* a, int localFrame, int globalFrame,
			int quantize, bool mixerIsRunning) override;
	bool canInputRec() override;
	bool allocBuffers() override;

	int getTrackerPreview() const;
	int getShift() const;
	float getBoost() const;	

	void setShift(int s);

	void reset(int frame);

	/* fade methods
	 * prepare channel for fade, mixer will take care of the process
	 * during master play. */

	void setFadeIn(bool internal);
	void setFadeOut(int actionPostFadeout);
	void setXFade(int frame);

	/* pushWave
	Adds a new wave to an existing channel. */

	void pushWave(Wave* w);

	/* getPosition
	 * returns the position of an active sample. If EMPTY o MISSING
	 * returns -1. */

	int getPosition();

	/* sum
	 * add sample frames to virtual channel. Frame = processed frame in
	 * Mixer. Running = is Mixer in play? */

	void sum(int frame, bool running);


	void setPitch(float v);
	float getPitch();
	void setBegin(int f);
	int getBegin();
	void setEnd(int f);
	int getEnd();
	void setTrackerPreview(int f);

	/* hardStop
	 * stop the channel immediately, no further checks. */

	void hardStop(int frame);

	/* setReadActions
	If enabled (v == true), recorder will read actions from this channel. If 
	killOnFalse == true and disabled, will also kill the channel. */

	void setReadActions(bool v, bool killOnFalse);

	void setBoost(float v);

	void setOnEndPreviewCb(std::function<void()> f);

	Wave* wave;
	int   tracker;         // chan position
	int   mode;            // mode: see const.h
	bool  qWait;           // quantizer wait
	bool  fadeinOn;
	float fadeinVol;
	bool  fadeoutOn;
	float fadeoutVol;      // fadeout volume
	int   fadeoutTracker;  // tracker fadeout, xfade only
	float fadeoutStep;     // fadeout decrease
  int   fadeoutType;     // xfade or fadeout
  int		fadeoutEnd;      // what to do when fadeout ends
  bool  inputMonitor;

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
