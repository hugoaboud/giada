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

#include "const.h"
#include "conf.h"
#include "pluginHost.h"
#include "inputChannel.h"
#include "columnChannel.h"
#include "../utils/log.h"
using namespace giada::m;

InputChannel::InputChannel(int bufferSize)
	: Channel          (bufferSize),
	inputIndex(-1),
	preMute(false),
	columnChannel(nullptr)
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
	return "i " + name;
}


/* -------------------------------------------------------------------------- */

void InputChannel::copy(const Channel *src, pthread_mutex_t *pluginMutex) {
	/*
		TODO
	*/
}

/* -------------------------------------------------------------------------- */

void InputChannel::parseAction(giada::m::recorder::action* a, int localFrame, int globalFrame, bool mixerIsRunning){
	/*
		TODO
	*/
}

/* -------------------------------------------------------------------------- */

void InputChannel::clearBuffers()
{
	std::memset(vChan, 0, sizeof(float) * bufferSize);
}

/* -------------------------------------------------------------------------- */

void InputChannel::input(float* inBuffer) {
	for (int i=0; i<bufferSize; i++) {
		vChan[i] += inBuffer[inputIndex+i*conf::channelsIn]; // add, don't overwrite (no raw memcpy)
		if (vChan[i] > peak) peak = vChan[i];
	}
}

/* -------------------------------------------------------------------------- */

void InputChannel::process(float* outBuffer, float* inBuffer) {
	
	/* If input monitor is on, copy input buffer to vChan: this enables the input
  monitoring. The vChan will be overwritten later by pluginHost::processStack. */
	bool chainAlive = isChainAlive();
	if ((chainAlive || inputMonitor) && inputIndex > -1)
	{
		if (!preMute)
	    	input(inBuffer);

	#ifdef WITH_VST
		pluginHost::processStack(vChan, this);
	#endif
	}

	// TODO - Opaque channels' processing
	if (inputMonitor) {
	  	for (int j=0; j<bufferSize; j++) {
			outBuffer[j*2]   += vChan[j] * volume * calcPanning(0);
			outBuffer[j*2+1] +=	vChan[j] * volume * calcPanning(1);
		}
	}

	// feed output ColumnChannel
	if (chainAlive && columnChannel != nullptr && !preMute && !mute) {
		columnChannel->input(vChan);
	}

	peak = 0;
}

/* -------------------------------------------------------------------------- */

bool InputChannel::isChainAlive() {
	if (columnChannel == nullptr) {
		if (!inputMonitor) return false;
		return true;
	}
	return columnChannel->isChainAlive();
}