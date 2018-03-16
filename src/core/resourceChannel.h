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


#ifndef G_RESOURCE_CHANNEL_H
#define G_RESOURCE_CHANNEL_H

#include "channel.h"

class ResourceChannel : public Channel
{
protected:

	/* previewMode
	Whether the channel is in audio preview mode or not. */
	int previewMode;

	/* onPreviewEnd
	A callback fired when audio preview ends. */
	std::function<void()> onPreviewEnd;

	float volume_i; // internal volume
	float volume_d; // delta volume (for envelope)

	int  type;                  // midi or sample
	int  status;                // status of resource ()
	int  recStatus;             // status of recordings (waiting, ending, ...)
	bool readActions;	        // read what's recorded
	bool armed;                 // armed for recording

public:

	ResourceChannel(int type, int status, int bufferSize);
	~ResourceChannel();

	// [ Channel inheritance ]
	virtual bool allocBuffers();
	virtual void copy(const Channel* src, pthread_mutex_t* pluginMutex) = 0;
	virtual int readPatch(const std::string& basePath, int i, pthread_mutex_t* pluginMutex, int samplerate, int rsmpQuality);
	virtual int writePatch(int i, bool isProject);

	/* Preview
	Makes itself audibile for audio preview, such as Sample Editor or other
	tools. */

	virtual void preview(float* outBuffer) = 0;

	/* start
	Action to do when channel starts. doQuantize = false (don't quantize)
	when Mixer is reading actions from Recorder. If isUserGenerated means that
	the channel has been started by a human key press and not a pre-recorded
	action. */

	virtual void start(int frame, bool doQuantize, bool mixerIsRunning, bool forceStart, bool isUserGenerated) = 0;

	/* stop
	What to do when channel is stopped normally (via key or MIDI). */

	virtual void stop() = 0;

	/* start
	Action to do when recording on this channel starts. doQuantize = false
	(don't quantize) when Mixer is reading actions from Recorder. If isUserGenerated
	means that the channel has been started by a human key press and not a pre-recorded action. */

	virtual void rec(int frame, bool doQuantize, bool mixerIsRunning, bool forceStart, bool isUserGenerated) = 0;
	
	/* stop
	What to do when recording on this channel is stopped normally (via key or MIDI). */

	virtual void recStart() = 0;

	/* stop
	What to do when recording on this channel is stopped normally (via key or MIDI). */

	virtual void recStop() = 0;

	/* kill
	What to do when channel stops abruptly. */

	virtual void kill(int frame) = 0;

	/* empty
	Frees any associated resources (e.g. waveform for SAMPLE). */

	virtual void empty() = 0;

	/* stopBySeq
	What to do when channel is stopped by sequencer. */

	virtual void stopBySeq(bool chansStopOnSeqHalt) = 0;

	/* quantize
	Starts channel according to quantizer. Index = array index of mixer::channels, 
	used by recorder. LocalFrame = frame within the current buffer.  */

	virtual void quantize(int index, int localFrame) = 0;

	/* onZero
	What to do when frame goes to zero, i.e. sequencer restart. */

	virtual void onZero(int frame, bool recsStopOnChanHalt) = 0;

	/* onBar
	What to do when a bar has passed. */

	virtual void onBar(int frame) = 0;

	/* rewind
	Rewinds channel when rewind button is pressed. */

	virtual void rewind() = 0;

	/* canInputRec
	Tells whether a channel can accept and handle input audio. Always false for
	Midi channels, true for Sample channels only if they don't contain a
	sample yet.*/

	virtual bool canInputRec() = 0;
	
	/* isPreview
	Whethet a channel is previewing. */
	bool isPreview();

	/* setReadActions
	If enabled (v == true), recorder will read actions from this channel. If 
	killOnFalse == true and disabled, will also kill the channel. */

	void setReadActions(bool v, bool killOnFalse);

	/* isPlaying
	If status == STATUS_PLAY | STATUS_ENDING return true. */
	
	bool isPlaying();

	/* sendMidiL*
	 * send MIDI lightning events to a physical device. */

	void sendMidiLmute();
	void sendMidiLsolo();
	void sendMidiLplay();

	void setVolumeI(float v);
	void setArmed(bool b);
	void setPreviewMode(int m);

	// midi stuff

	uint32_t midiInKeyPress;
	uint32_t midiInKeyRel;
	uint32_t midiInKill;
	uint32_t midiInArm;
	
	uint32_t midiOutLplaying;

	bool     midiInVeloAsVol;
	uint32_t midiInReadActions;
	uint32_t midiInPitch;

	/* setters & getters */
	
	bool isArmed() const { return armed; };
	int getType() const { return type; }
	int getStatus() const { return status; };
	int getRecStatus() const { return recStatus; };
	int getReadActions() const { return readActions; };
};


#endif