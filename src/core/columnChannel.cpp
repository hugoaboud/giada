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
#include "columnChannel.h"
#include "pluginHost.h"
#include "conf.h"

using namespace giada::m;


ColumnChannel::ColumnChannel(int bufferSize)
	: Channel          (CHANNEL_SAMPLE, STATUS_EMPTY, bufferSize)
{
}


/* -------------------------------------------------------------------------- */


ColumnChannel::~ColumnChannel()
{
}


/* -------------------------------------------------------------------------- */

std::string ColumnChannel::getName() const
{
	return "c." + name;
}


/* -------------------------------------------------------------------------- */

void ColumnChannel::copy(const Channel *src, pthread_mutex_t *pluginMutex) {}
void ColumnChannel::preview(float* outBuffer) {}
void ColumnChannel::start(int frame, bool doQuantize, int quantize, bool mixerIsRunning, bool forceStart, bool isUserGenerated) {}
void ColumnChannel::stop() {}
void ColumnChannel::kill(int frame) {}
void ColumnChannel::setMute  (bool internal) {}
void ColumnChannel::unsetMute(bool internal) {}
void ColumnChannel::empty() {}
void ColumnChannel::stopBySeq(bool chansStopOnSeqHalt) {}
void ColumnChannel::quantize(int index, int localFrame) {}
void ColumnChannel::onZero(int frame, bool recsStopOnChanHalt) {}
void ColumnChannel::onBar(int frame) {}
void ColumnChannel::parseAction(giada::m::recorder::action* a, int localFrame, int globalFrame, int quantize, bool mixerIsRunning){}
void ColumnChannel::rewind() {}

void ColumnChannel::clear() {
	/** TODO - these memsets may be done only if status PLAY (if below),
	 * but it would require extra clearPChan calls when samples stop */
	std::memset(vChan, 0, sizeof(float) * bufferSize);
}

void ColumnChannel::input(float* inBuffer) {
	if (canInputRec() && index > -1) {
		for (int i=0; i<bufferSize; i++) {
			vChan[i] += inBuffer[i]; // add, don't overwrite (no raw memcpy)
			if (vChan[i] > peak) peak = vChan[i];
		}
	}
}

void ColumnChannel::process(float* outBuffer, float* inBuffer) {
	for (unsigned i=0; i<samples.size(); i++) {
		samples[i]->process(outBuffer, vChan);
		samples[i]->preview(outBuffer);
	}

#ifdef WITH_VST
	pluginHost::processStack(outBuffer, this);
#endif

	for (int i=0; i<bufferSize; i++)
		outBuffer[i] *= volume;

}

bool ColumnChannel::canInputRec() {
	for (unsigned i = 0; i < samples.size(); i++) if (samples[i]->isArmed()) return true;
	return false;
}

SampleChannel* ColumnChannel::getSample(int index) {
	return samples[index];
}

void ColumnChannel::addSample(SampleChannel* sample) {
	samples.push_back(sample);
}