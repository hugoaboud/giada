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

Channel::Channel(int type, int bufferSize)
: type					(type),
	bufferSize    (bufferSize),
	volume_i      (0.0f),
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
	if (!vChan.alloc(bufferSize, G_MAX_IO_CHANS)) {
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


void Channel::writePatch(int i, bool isProject)
{
	channelManager::writePatch(this, isProject);
}


/* -------------------------------------------------------------------------- */


void Channel::readPatch(const string& path, int i)
{
	channelManager::readPatch(this, i);
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
	assert(in.countSamples() == vChan.countSamples());
	assert(!pre_mute);

	/* Copy input buffer to vChan.
	The vChan will be overwritten later by pluginHost::processStack,
  so that you would record "clean" audio (i.e. not plugin-processed). */

	if (in.isAllocd()) {
		peak = 0;
		for (int i=0; i<vChan.countFrames(); i++)
			for (int j=0; j<vChan.countChannels(); j++) {
				vChan[i][j] += in[i][j];   // add, don't overwrite
				if (vChan[i][j] > peak) peak = vChan[i][j];
			}
	}
}

void Channel::output(giada::m::AudioBuffer& out) {
	assert(out.countSamples() == vChan.countSamples());
	for (int i=0; i<out.countFrames(); i++)
		for (int j=0; j<out.countChannels(); j++)
			out[i][j] += vChan[i][j] * volume * calcPanning(j) * boost;
}

bool Channel::isNodeAlive() {
	return inputMonitor && !mute;
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
