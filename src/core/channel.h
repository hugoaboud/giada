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


#ifndef G_CHANNEL_H
#define G_CHANNEL_H


#include <vector>
#include <string>
#include <pthread.h>
#include "midiMapConf.h"
#include "midiEvent.h"
#include "recorder.h"

#ifdef WITH_VST
	#include "../deps/juce-config.h"
#endif


class Plugin;
class MidiMapConf;
class geChannel;


class Channel
{
protected:

	/* sendMidiLMessage
	Composes a MIDI message by merging bytes from MidiMap conf class, and sends it 
	to KernelMidi. */

	void sendMidiLmessage(uint32_t learn, const giada::m::midimap::message_t& msg);

	/* calcPanning
	Given an audio channel (stereo: 0 or 1) computes the current panning value. */

	float calcPanning(int ch);

#ifdef WITH_VST

	/* MidiBuffer contains MIDI events. When ready, events are sent to each plugin 
	in the channel. This is available for any kind of channel, but it makes sense 
	only for MIDI channels. */

	juce::MidiBuffer midiBuffer;

#endif

	/* bufferSize
	Size of every buffer in this channel (vChan, pChan) */

	int bufferSize;

	/* midiInFilter
	Which MIDI channel should be filtered out when receiving MIDI messages. -1
	means 'all'. */

  	int midiInFilter;

	float pan;
	float volume;   // global volume
	float peak;
	
	std::string name;

public:

	Channel(int bufferSize);

	virtual ~Channel();

	/* copy
	Makes a shallow copy (no vChan/pChan allocation) of another channel. */

	virtual void copy(const Channel* src, pthread_mutex_t* pluginMutex) = 0;

	/* readPatch
	Fills channel with data from patch. */

	virtual int readPatch(const std::string& basePath, int i,
    pthread_mutex_t* pluginMutex, int samplerate, int rsmpQuality);

	/* isChainAlive
	Checks whether it's necessary to input/process audio (at some point in the
	chain, monitoring/recording requires this channel audio). */
	virtual bool isChainAlive() = 0;

	/* input
	Merges vChannels into buffer. Warning:
	inBuffer might be nullptr if no input devices are available for recording. */

	virtual void input(float* inBuffer) = 0;

	/* process
	Merges vChannels into buffer, plus plugin processing (if any). Warning:
	inBuffer might be nullptr if no input devices are available for recording. */

	virtual void process(float* outBuffer, float* inBuffer) = 0;

	/* mute / premute
	What to do when channel is muted. If internal == true, set internal mute 
	without altering main mute. 
	Pre: before FX*/

	virtual void setMute  (bool internal);
	virtual void unsetMute(bool internal);
	virtual void setPreMute(bool internal);
	virtual void unsetPreMute(bool internal);

	/* parseAction
	Does something on a recorded action. Parameters:
		- action *a   - action to parse
	  - localFrame  - frame number of the processed buffer
	  - globalFrame - actual frame in Mixer */

	virtual void parseAction(giada::m::recorder::action* a, int localFrame,
    int globalFrame, bool mixerIsRunning) = 0;

	/* writePatch
	Fills a patch with channel values. Returns the index of the last 
	Patch::channel_t added. */

	virtual int writePatch(int i, bool isProject);

	/* receiveMidi
	Receives and processes midi messages from external devices. */

	virtual void receiveMidi(const giada::m::MidiEvent& midiEvent);

	/* allocBuffers
	Mandatory method to allocate memory for internal buffers. Call it after the
	object has been constructed. */

	virtual bool allocBuffers();

	/* clear
	Clears all memory buffers. */

	virtual void clearBuffers();

	/* isMidiAllowed
	Given a MIDI channel 'c' tells whether this channel should be allowed to receive
	and process MIDI events on MIDI channel 'c'. */

	bool isMidiInAllowed(int c) const;

	/* setReadActions
	If enabled (v == true), recorder will read actions from this channel. If 
	killOnFalse == true and disabled, will also kill the channel. */

	virtual void setReadActions(bool v);

	/* sendMidiL*
	 * send MIDI lightning events to a physical device. */

	void sendMidiLmute();
	void sendMidiLsolo();
	void sendMidiLplay();

	void setName(const std::string& s);
	void setPan(float v);
	void setVolume(float v);
	void setMidiInFilter(int c);

	virtual std::string getName() const;
	float 	getPan() const;
	float 	getVolume() const;
	float 	getPeak() const;
	int 	getMidiInFilter() const;

#ifdef WITH_VST

	/* getPluginMidiEvents
	 * Return a reference to midiBuffer stack. This is available for any kind of
	 * channel, but it makes sense only for MIDI channels. */

	juce::MidiBuffer& getPluginMidiEvents();

	void clearMidiBuffer();

#endif

	int		index;			// unique id
	int 	key;			// keyboard button
	bool	mute_i;			// internal mute
	bool	mute_s;			// previous mute status after being solo'd
	bool	mute;			// global mute
	bool	pre_mute;	    // global pre mute
	bool	solo;			// global solo
	bool	inputMonitor;	// input monitor (copies processed input to output)
	bool	hasActions;		// has something recorded
	bool	readActions;	// read what's recorded
	float*	vChan;			// virtual channel
  	
  	geChannel* guiChannel;	// pointer to a gChannel object, part of the GUI

	// TODO - midi structs, please

	bool		midiIn;              // enable midi input
	uint32_t	midiInVolume;
	uint32_t	midiInMute;
	uint32_t	midiInSolo;

	/*  midiOutL*
	 * Enable MIDI lightning output, plus a set of midi lighting event to be sent
	 * to a device. Those events basically contains the MIDI channel, everything
	 * else gets stripped out. */

	bool		midiOutL;
	uint32_t	midiOutLplaying;
	uint32_t	midiOutLmute;
	uint32_t	midiOutLsolo;

#ifdef WITH_VST
  std::vector <Plugin*> plugins;
#endif

};


#endif
