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


#ifndef G_INPUT_CHANNEL_H
#define G_INPUT_CHANNEL_H

#include "channel.h"

class InputChannel : public Channel
{
private:

		std::string label;

public:

	InputChannel(int bufferSize);
	~InputChannel();

	std::string getLabel();

	void copy(const Channel *src, pthread_mutex_t *pluginMutex) override;
	void process(float* outBuffer, float* inBuffer) override;
	void preview(float* outBuffer) override;
	void start(int frame, bool doQuantize, int quantize, bool mixerIsRunning, bool forceStart, bool isUserGenerated) override;
	void stop() override;
	void kill(int frame) override;
	void setMute  (bool internal) override;
	void unsetMute(bool internal) override;
	void empty() override;
	void stopBySeq(bool chansStopOnSeqHalt) override;
	void quantize(int index, int localFrame) override;
	void onZero(int frame, bool recsStopOnChanHalt) override;
	void onBar(int frame) override;
	void parseAction(giada::m::recorder::action* a, int localFrame, int globalFrame, int quantize, bool mixerIsRunning) override;
	void rewind() override;
	void clear() override;
	bool canInputRec() override;
};

#endif
