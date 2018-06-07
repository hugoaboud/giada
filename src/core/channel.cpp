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
#include "../utils/log.h"
#include "../gui/elems/mainWindow/keyboard/channel.h"
#include "const.h"
#include "channelManager.h"
#include "pluginHost.h"
#include "plugin.h"
#include "kernelMidi.h"
#include "patch.h"
#include "clock.h"
#include "wave.h"
#include "mixer.h"
#include "mixerHandler.h"
#include "conf.h"
#include "patch.h"
#include "waveFx.h"
#include "midiMapConf.h"
#include "channel.h"


using std::string;
using namespace giada::m;

Channel::Channel(int type, int bufferSize, bool mono)
: type					(type),
	mono					(mono),
	bufferSize    (bufferSize),
	volume_i      (1.0f),
	volume_d      (0.0f),
	boost         (G_DEFAULT_BOOST),
	mute_i        (false),
	name					(""),
	volume        (G_DEFAULT_VOL),
	pan           (0.5f),
	peak					(0),
	mute          (false),
	mute_s				(false),
	pre_mute			(false),
	solo          (false),
	inputMonitor  (false),
	key           (0),
	readActions   (false),
	hasActions    (false),
	guiChannel    (nullptr),
	midiIn        (true),
	midiInVolume  (0x0),
	midiInMute    (0x0),
	midiInSolo    (0x0),
	midiInFilter  (-1),
	midiOutL      (false),
	midiOutLmute  (0x0),
	midiOutLsolo  (0x0)
{
}


/* -------------------------------------------------------------------------- */


Channel::~Channel() {}


/* -------------------------------------------------------------------------- */


bool Channel::allocBuffers()
{
	if (!vChan.alloc(bufferSize, mono?1:2)) {
		gu_log("[Channel::allocBuffers] unable to alloc memory for vChan!\n");
		return false;
	}
	return true;
}

/* -------------------------------------------------------------------------- */

void Channel::clearBuffers() {
	vChan.clear();
}


/* -------------------------------------------------------------------------- */


void Channel::copy(const Channel* src, pthread_mutex_t* pluginMutex)
{
	key             = src->key;
	volume          = src->volume;
	pan             = src->pan;
	mute_i          = src->mute_i;
	mute_s          = src->mute_s;
	mute            = src->mute;
	solo            = src->solo;
	hasActions      = src->hasActions;
	midiIn          = src->midiIn;
	midiInVolume    = src->midiInVolume;
	midiInMute      = src->midiInMute;
	midiInSolo      = src->midiInSolo;
	midiOutLplaying = src->midiOutLplaying;

	/* clone plugins */

#ifdef WITH_VST
	for (unsigned i=0; i<src->plugins.size(); i++)
		pluginHost::clonePlugin(src->plugins.at(i), pluginMutex, this);
#endif

	/* clone actions */

	for (unsigned i=0; i<recorder::global.size(); i++) {
		for (unsigned k=0; k<recorder::global.at(i).size(); k++) {
			recorder::action* a = recorder::global.at(i).at(k);
			if (a->chan == src->index) {
				recorder::rec(index, a->type, a->frame, a->iValue, a->fValue);
				hasActions = true;
			}
		}
	}
}


/* -------------------------------------------------------------------------- */


void Channel::sendMidiLmessage(uint32_t learn, const midimap::message_t& msg)
{
	gu_log("[channel::sendMidiLmessage] learn=%#X, chan=%d, msg=%#X, offset=%d\n",
		learn, msg.channel, msg.value, msg.offset);

	/* isolate 'channel' from learnt message and offset it as requested by 'nn'
	 * in the midimap configuration file. */

		uint32_t out = ((learn & 0x00FF0000) >> 16) << msg.offset;

	/* merge the previously prepared channel into final message, and finally
	 * send it. */

	out |= msg.value | (msg.channel << 24);
	kernelMidi::send(out);
}

/* -------------------------------------------------------------------------- */


void Channel::writePatch(bool isProject)
{
	channelManager::writePatch(this, isProject);
}


/* -------------------------------------------------------------------------- */


void Channel::readPatch(const string& path, int i)
{
	channelManager::readPatch(this, i);
}

/* -------------------------------------------------------------------------- */

void Channel::setMono(bool mono) {
	if (mono != this->mono) {
		this->mono = mono;
		allocBuffers();
	}
}

/* -------------------------------------------------------------------------- */

void Channel::toggleMono() {
	setMono(!mono);
}

/* -------------------------------------------------------------------------- */

bool Channel::isMono() {
	return mono;
}

/* -------------------------------------------------------------------------- */

void Channel::setReadActions(bool r) {
	readActions = r;
}

/* -------------------------------------------------------------------------- */


void Channel::sendMidiLmute()
{
	if (!midiOutL || midiOutLmute == 0x0)
		return;
	if (mute)
		sendMidiLmessage(midiOutLsolo, midimap::muteOn);
	else
		sendMidiLmessage(midiOutLsolo, midimap::muteOff);
}


/* -------------------------------------------------------------------------- */


void Channel::sendMidiLsolo()
{
	if (!midiOutL || midiOutLsolo == 0x0)
		return;
	if (solo)
		sendMidiLmessage(midiOutLsolo, midimap::soloOn);
	else
		sendMidiLmessage(midiOutLsolo, midimap::soloOff);
}


/* -------------------------------------------------------------------------- */


void Channel::receiveMidi(const MidiEvent& midiEvent)
{
}


/* -------------------------------------------------------------------------- */


bool Channel::isMidiInAllowed(int c) const
{
	return midiInFilter == -1 || midiInFilter == c;
}

/* -------------------------------------------------------------------------- */

void Channel::input(giada::m::AudioBuffer& in)
{
	if (pre_mute || !in.isAllocd()) return;
	assert(in.countFrames() == vChan.countFrames());

	bool mono_in = in.countChannels() == 1;

	/* Add input buffer to vChan.
	The vChan will be overwritten later by pluginHost::processStack,
  so that you would record "clean" audio (i.e. not plugin-processed).
	If input is mono(L) and channel is stereo(L,R), the result is (L,L);
	If input is stereo(L,R) and channel is mono(L), the result is ((L+R)/2) */

	if (mono) {
		if (mono_in) { // mono channel, mono input
			for (int i=0; i<vChan.countFrames(); i++)
				vChan[i][0] += in[i][0];
		}
		else { // mono channel, stereo input
			for (int i=0; i<vChan.countFrames(); i++)
				vChan[i][0] += (in[i][0] + in[i][1])/2;
		}
	}
	else {
		if (mono_in) { // stereo channel, mono input
				for (int i=0; i<vChan.countFrames(); i++) {
					vChan[i][0] += in[i][0];
					vChan[i][1] += in[i][0];
				}
		}
		else { // stereo channel, stereo input
				for (int i=0; i<vChan.countFrames(); i++) {
					vChan[i][0] += in[i][0];
					vChan[i][1] += in[i][1];
				}
		}
	}
}

void Channel::output(giada::m::AudioBuffer& out) {
	if (mute) return;
	assert(out.countFrames() == vChan.countFrames());

	// Output fixed to stereo
	peak = 0;
	int outCh = conf::channelsOut;
	if (mono) {
		for (int i=0; i<vChan.countFrames(); i++) {
			out[i][outCh] += vChan[i][0] * volume * calcPanning(0) * boost;
			out[i][outCh+1] += vChan[i][0] * volume * calcPanning(1) * boost;
			if (out[i][outCh] > peak) peak = out[i][outCh];
			if (out[i][outCh+1] > peak) peak = out[i][outCh+1];
		}
	}
	else {
		for (int i=0; i<vChan.countFrames(); i++) {
			out[i][outCh] += vChan[i][0] * volume * calcPanning(0) * boost;
			out[i][outCh+1] += vChan[i][1] * volume * calcPanning(1) * boost;
			if (out[i][outCh] > peak) peak = out[i][outCh];
			if (out[i][outCh+1] > peak) peak = out[i][outCh+1];
		}
	}
}

/* -------------------------------------------------------------------------- */


void Channel::setPan(float v)
{
	if (v > 1.0f)
		pan = 1.0f;
	else
	if (v < 0.0f)
		pan = 0.0f;
	else
		pan = v;
}


float Channel::getPan() const
{
	return pan;
}


float Channel::getPeak() const
{
	return peak;
}

/* -------------------------------------------------------------------------- */

void Channel::setMute  (bool internal) {
	pre_mute = false;
	mute = true;
}
void Channel::unsetMute(bool internal) {
	mute = false;
}
void Channel::setPreMute  (bool internal) {
	pre_mute = true;
	mute = false;
}
void Channel::unsetPreMute(bool internal) {
	pre_mute = false;
}

/* -------------------------------------------------------------------------- */

void Channel::setVolume(float v)
{
	volume = v;
}

float Channel::getVolume() const
{
	return volume;
}

/* -------------------------------------------------------------------------- */

void Channel::setBoost(float v)
{
	if (v > G_MAX_BOOST_DB)
		boost = G_MAX_BOOST_DB;
	else
	if (v < 0.0f)
		boost = 0.0f;
	else
		boost = v;
}


float Channel::getBoost() const
{
	return boost;
}

/* -------------------------------------------------------------------------- */


float Channel::calcPanning(int ch)
{
	if (pan == 0.5f) // center: nothing to do
		return 1.0;
	if (ch == 0)
		return 1.0 - pan;
	else  // channel 1
		return pan;
}

/* -------------------------------------------------------------------------- */


std::string Channel::getName() const
{
	return name;
}


void Channel::setName(const std::string& s)
{
	name = s;
}


/* -------------------------------------------------------------------------- */


#ifdef WITH_VST

juce::MidiBuffer &Channel::getPluginMidiEvents()
{
	return midiBuffer;
}


/* -------------------------------------------------------------------------- */


void Channel::clearMidiBuffer()
{
	midiBuffer.clear();
}


#endif
