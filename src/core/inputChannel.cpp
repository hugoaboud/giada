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
#include "conf.h"
#include "pluginHost.h"
#include "inputChannel.h"
#include "columnChannel.h"
#include "../utils/log.h"
using namespace giada::m;

InputChannel::InputChannel(int bufferSize)
	: Channel          (G_CHANNEL_INPUT, bufferSize),
	inputIndex(-1),
	outColumn(nullptr)
{
	name = "Input";
	inputMonitor = conf::inputMonitorDefaultOn;
}


/* -------------------------------------------------------------------------- */

InputChannel::~InputChannel()
{}

/* -------------------------------------------------------------------------- */

std::string InputChannel::getName() const
{
	return ">" + name;
}


/* -------------------------------------------------------------------------- */

void InputChannel::copy(const Channel *src, pthread_mutex_t *pluginMutex) {
	/*
		TODO
	*/
}

/* -------------------------------------------------------------------------- */


void InputChannel::writePatch(int i, bool isProject)
{
	/*
		TODO
	*/
}


/* -------------------------------------------------------------------------- */


void InputChannel::readPatch(const std::string& basePath, int i)
{
	/*
		TODO
	*/
}

/* -------------------------------------------------------------------------- */

void InputChannel::process(giada::m::AudioBuffer& out, giada::m::AudioBuffer& in)
{
	if (inputIndex < 0 || !isNodeAlive()) return;
	assert(out.countSamples() == vChan.countSamples());
	assert(in.countSamples() == vChan.countSamples());

	input(in);

#ifdef WITH_VST
		pluginHost::processStack(vChan, this);
#endif

	if (inputMonitor)
		output(out);

	// feed outColumn ColumnChannel
	if (outColumn != nullptr && outColumn->isNodeAlive()) {
		outColumn->input(vChan);
	}
}

void InputChannel::parseAction(giada::m::recorder::action* a, int localFrame, int globalFrame, bool mixerIsRunning) {

}

/* -------------------------------------------------------------------------- */

bool InputChannel::isNodeAlive() {
	if (mute || pre_mute) return false;
	if (inputMonitor) return true;
	if (outColumn == nullptr) return false;
	return outColumn->isNodeAlive();
}
