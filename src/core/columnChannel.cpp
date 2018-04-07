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

std::string ColumnChannel::getName() const
{
	return "c " + name;
}


/* -------------------------------------------------------------------------- */

void ColumnChannel::copy(const Channel *src, pthread_mutex_t *pluginMutex) {
	/*
		TODO
	*/
}

/* -------------------------------------------------------------------------- */


void ColumnChannel::writePatch(int i, bool isProject)
{
	/*
		TODO
	*/
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
	if (!isNodeAlive()) return;

	assert(out.countSamples() == vChan.countSamples());
	// Ignore input, receive only throught ColumnChannel::input()

	if (!pre_mute) {
		for (unsigned i=0; i<resources.size(); i++) {
			resources[i]->process(out, vChan);
			resources[i]->preview(out);
		}
	}

#ifdef WITH_VST
	pluginHost::processStack(vChan, this);
#endif

	if (inputMonitor)
		output(out);
}

/* -------------------------------------------------------------------------- */


bool ColumnChannel::isNodeAlive() {
	if (mute) return false;
	if (inputMonitor) return true;
	for (unsigned i = 0; i < resources.size(); i++) if (resources[i]->isNodeAlive()) return true;
	return false;
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
	for (unsigned i=0; i<resources.size(); i++) {
		ResourceChannel* ch = mixer::columnChannels[i]->getResource(i);
		if (ch->armed && ch->recStatus == REC_STOPPED) {
			ch->recStart();
		}
	}
}

void ColumnChannel::stopRecResources() {
	for (unsigned i=0; i<resources.size(); i++) {
		ResourceChannel* ch = mixer::columnChannels[i]->getResource(i);
		if (ch->armed || ch->recStatus != REC_STOPPED) {
			ch->recStop();
		}
	}
}

void ColumnChannel::clearAllResources() {
	for (unsigned i = 0; i < resources.size(); i++) {
		resources.at(i)->empty();
		resources.at(i)->guiChannel->reset();
	}
}

bool ColumnChannel::isSilent() {
	for (unsigned i = 0; i < resources.size(); i++) {
		int status = resources.at(i)->status;
		if (status == STATUS_PLAY || status == STATUS_ENDING) return false;
	}
	return true;
}

/* -------------------------------------------------------------------------- */
