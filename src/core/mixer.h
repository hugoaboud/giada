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


#ifndef G_MIXER_H
#define G_MIXER_H


#include <pthread.h>
#include <vector>
#include "../deps/rtaudio-mod/RtAudio.h"


class Channel;
class InputChannel;
class ColumnChannel;


namespace giada {
namespace m {
namespace mixer
{
void init(int framesInSeq, int audioBufferSize);

int close();

/* masterPlay
Core method (callback) */

int masterPlay(void *outBuf, void *inBuf, unsigned bufferSize, double streamTime,
  RtAudioStreamStatus status, void *userData);

/* isSilent
Is mixer silent? */

bool isSilent();

/* rewind
Rewinds sequencer to frame 0. */

void rewind();

/* mergeVirtualInput
Copies the virtual channel input in the channels designed for input recording. 
Called by mixerHandler on stopInputRec(). */

void mergeVirtualInput();

enum {    // const - what to do when a fadeout ends
	DO_STOP   = 0x01,
	DO_MUTE   = 0x02,
	DO_MUTE_I = 0x04
};

enum {    // const - fade types
	FADEOUT = 0x01,
	XFADE   = 0x02
};

extern std::vector<InputChannel*> inputChannels;
extern std::vector<ColumnChannel*> columnChannels;
extern std::vector<Channel*> channels; // this is about to disappear
//extern std::vector<MasterChannel*> masterChannels;

extern bool   recording;         // is recording something?
extern bool   ready;
extern int    frameSize;
extern float  outVol;
extern float  inVol;
extern float  peakIn;
extern float  peakOut;
extern bool	 metronome;
extern int    waitRec;      // delayComp guard
extern bool  docross;			 // crossfade guard
extern bool  rewindWait;	   // rewind guard, if quantized

extern int  tickTracker, tockTracker;
extern bool tickPlay, tockPlay; // 1 = play, 0 = stop

extern pthread_mutex_t mutex_recs;
extern pthread_mutex_t mutex_chans;
extern pthread_mutex_t mutex_plugins;

}}} // giada::m::mixer::;


#endif
