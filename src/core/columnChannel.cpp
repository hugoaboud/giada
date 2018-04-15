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
#include "const.h"
#include "sampleChannel.h"
#include "columnChannel.h"
#include "pluginHost.h"
#include "conf.h"
#include "mixer.h"
#include "clock.h"
#include "../utils/log.h"
#include "../gui/elems/mainWindow/keyboard/channel.h"

using namespace giada::m;


ColumnChannel::ColumnChannel(int bufferSize)
	: Channel(G_CHANNEL_COLUMN, bufferSize),
	outputIndex(-1)
{
}


/* -------------------------------------------------------------------------- */


ColumnChannel::~ColumnChannel()
{
}

/* -------------------------------------------------------------------------- */

void ColumnChannel::copy(const Channel *src, pthread_mutex_t *pluginMutex) {
	/*
		TODO
	*/
}

/* -------------------------------------------------------------------------- */


void ColumnChannel::writePatch(bool isProject)
{
	/*
		TODO
	*/
}

/* -------------------------------------------------------------------------- */


bool ColumnChannel::allocBuffers()
{
	if (!Channel::allocBuffers())
		return false;

	if (!rChan.alloc(bufferSize, mono?1:2)) {
		gu_log("[ColumnChannel::allocBuffers] unable to alloc memory for rChan!\n");
		return false;
	}

	return true;
}

/* -------------------------------------------------------------------------- */


void ColumnChannel::clearBuffers()
{
	vChan.clear();
	rChan.clear();
}

/* -------------------------------------------------------------------------- */

void ColumnChannel::readPatch(const std::string& basePath, int i)
{
	/*
		TODO
	*/
}

/* -------------------------------------------------------------------------- */

void ColumnChannel::parseAction(giada::m::recorder::action* a, int localFrame, int globalFrame, bool mixerIsRunning){
	/*
		TODO
	*/
}

/* -------------------------------------------------------------------------- */

void ColumnChannel::process(giada::m::AudioBuffer& out, giada::m::AudioBuffer& in)
{
	if (mute) return;

	assert(out.countFrames() == vChan.countFrames());
	// Ignore input, receive only throught ColumnChannel::input()

	bool rAlive = false;
	if (!pre_mute) {
		for (ResourceChannel* ch : resources) {
			if (!rAlive)
				if (ch->inputMonitor || ch->isPlaying() || ch->isRecording()) rAlive = true;
			ch->process(rChan, vChan);
			ch->preview(rChan);
		}
	}

	if (!inputMonitor) vChan.clear();

	if (rAlive) {
		for (int i=0; i<vChan.countFrames(); i++) {
			for (int j=0; j<vChan.countChannels(); j++) {
				vChan[i][j] += rChan[i][j];
			}
		}
	}

#ifdef WITH_VST
	pluginHost::processStack(vChan, this);
#endif

		output(out);
}

/* -------------------------------------------------------------------------- */

ResourceChannel* ColumnChannel::getResource(int index) {
	return resources[index];
}

void ColumnChannel::addResource(ResourceChannel* sample) {
	resources.push_back(sample);
}

void ColumnChannel::removeResource(int index) {
	resources.erase(resources.begin() + index);
}

unsigned ColumnChannel::getResourceCount() {
	return resources.size();
}

/* -------------------------------------------------------------------------- */

void ColumnChannel::recArmedResources() {
	gu_log("recArmedResources\n");
	for (ResourceChannel* ch : resources) {
		if (ch->armed && ch->recStatus == REC_STOPPED) {
			ch->recStart();
		}
	}
}

void ColumnChannel::stopRecResources() {
	for (ResourceChannel* ch : resources) {
		if (ch->armed || ch->recStatus != REC_STOPPED) {
			ch->recStop();
		}
	}
}

void ColumnChannel::clearAllResources() {
	for (ResourceChannel* ch : resources) {
		ch->empty();
		ch->guiChannel->reset();
	}
}

bool ColumnChannel::isSilent() {
	for (ResourceChannel* ch : resources) {
		int status = ch->status;
		if (status == STATUS_PLAY || status == STATUS_ENDING) return false;
	}
	return true;
}

/* -------------------------------------------------------------------------- */

void ColumnChannel::setMono(bool mono) {
	rChan.free();
	Channel::setMono(mono);
}
