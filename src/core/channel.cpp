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
#include "channel.h"
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


using std::string;
using namespace giada::m;


Channel::Channel(int bufferSize)
: bufferSize     (bufferSize),
	midiInFilter   (-1),
	pan            (0.5f),
	volume         (G_DEFAULT_VOL),
	key            (0),
	mute_i         (false),
	mute_s         (false),
	mute           (false),
	pre_mute           (false),
	solo           (false),
	inputMonitor   (false),
	hasActions     (false),
	vChan          (nullptr),
	midiIn         (true),
	midiInVolume   (0x0),
	midiInMute     (0x0),
	midiInSolo     (0x0),
	midiOutL       (false),
	midiOutLmute   (0x0),
	midiOutLsolo   (0x0)
{
}


/* -------------------------------------------------------------------------- */


Channel::~Channel()
{
	if (vChan != nullptr)
		delete[] vChan;
}


/* -------------------------------------------------------------------------- */


bool Channel::allocBuffers()
{
	vChan = new (std::nothrow) float[bufferSize];
	if (vChan == nullptr) {
		gu_log("[Channel::allocBuffers] unable to alloc memory for vChan!\n");
		return false;
	}
	std::memset(vChan, 0, bufferSize * sizeof(float));	
	return true;
}

/* -------------------------------------------------------------------------- */

void Channel::clearBuffers() {
	std::memset(vChan, 0, sizeof(float) * bufferSize);
}


/* -------------------------------------------------------------------------- */


void Channel::copy(const Channel *src, pthread_mutex_t *pluginMutex)
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
			recorder::action *a = recorder::global.at(i).at(k);
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


int Channel::writePatch(int i, bool isProject)
{
	patch::channel_t pch;
	pch.index           = index;
	pch.name            = name;
	pch.key             = key;
	pch.mute            = mute;
	pch.mute_s          = mute_s;
	pch.solo            = solo;
	pch.volume          = volume;
	pch.pan             = pan;
	pch.midiIn          = midiIn;
	pch.midiInVolume    = midiInVolume;
	pch.midiInMute      = midiInMute;
	pch.midiInFilter    = midiInFilter;
	pch.midiInSolo      = midiInSolo;
	pch.midiOutL        = midiOutL;
	pch.midiOutLmute    = midiOutLmute;
	pch.midiOutLsolo    = midiOutLsolo;

	for (unsigned i=0; i<recorder::global.size(); i++) {
		for (unsigned k=0; k<recorder::global.at(i).size(); k++) {
			recorder::action* action = recorder::global.at(i).at(k);
			if (action->chan == index) {
				patch::action_t pac;
				pac.type   = action->type;
				pac.frame  = action->frame;
				pac.fValue = action->fValue;
				pac.iValue = action->iValue;
				pch.actions.push_back(pac);
			}
		}
	}

#ifdef WITH_VST

	unsigned numPlugs = pluginHost::countPlugins(this);
	for (unsigned i=0; i<numPlugs; i++) {
		Plugin* pPlugin = pluginHost::getPluginByIndex(i, this);
		patch::plugin_t pp;
		pp.path   = pPlugin->getUniqueId();
		pp.bypass = pPlugin->isBypassed();
		for (int k=0; k<pPlugin->getNumParameters(); k++)
			pp.params.push_back(pPlugin->getParameter(k));
		for (unsigned k=0; k<pPlugin->midiInParams.size(); k++)
			pp.midiInParams.push_back(pPlugin->midiInParams.at(k));
		pch.plugins.push_back(pp);
	}

#endif

	patch::channels.push_back(pch);

	return patch::channels.size() - 1;
}


/* -------------------------------------------------------------------------- */

int Channel::readPatch(const string& path, int i, pthread_mutex_t* pluginMutex,
	int samplerate, int rsmpQuality)
{
	int ret = 1;
	patch::channel_t* pch = &patch::channels.at(i);
	key             = pch->key;
	name            = pch->name;
	index           = pch->index;
	mute            = pch->mute;
	mute_s          = pch->mute_s;
	solo            = pch->solo;
	volume          = pch->volume;
	pan             = pch->pan;
	midiIn          = pch->midiIn;
	midiInVolume    = pch->midiInVolume;
	midiInMute      = pch->midiInMute;
	midiInFilter    = pch->midiInFilter;
	midiInSolo      = pch->midiInSolo;
	midiOutL        = pch->midiOutL;
	midiOutLmute    = pch->midiOutLmute;
	midiOutLsolo    = pch->midiOutLsolo;

	for (const patch::action_t& ac : pch->actions) {
		recorder::rec(index, ac.type, ac.frame, ac.iValue, ac.fValue);
		hasActions = true;
	}

#ifdef WITH_VST

	for (const patch::plugin_t& ppl : pch->plugins) {
		
		Plugin* plugin = pluginHost::addPlugin(ppl.path, 			pluginMutex, this);
		
		if (plugin == nullptr) {
			ret &= 0;
			continue;
		}

		plugin->setBypass(ppl.bypass);
		
		for (unsigned j=0; j<ppl.params.size(); j++)
			plugin->setParameter(j, ppl.params.at(j));
		
		/* Don't fill Channel::midiInParam if Patch::midiInParams are 0: it would
		wipe out the current default 0x0 values. */

		if (!ppl.midiInParams.empty()) {
			plugin->midiInParams.clear();
			for (uint32_t midiInParam : ppl.midiInParams)
				plugin->midiInParams.push_back(midiInParam);
		}

		ret &= 1;
	}

#endif

	return ret;
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


void Channel::setMidiInFilter(int c)
{
	midiInFilter = c;
}


int Channel::getMidiInFilter() const
{
	return midiInFilter;
}


bool Channel::isMidiInAllowed(int c) const
{
	return midiInFilter == -1 || midiInFilter == c;
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
