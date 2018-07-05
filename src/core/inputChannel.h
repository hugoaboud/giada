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

public:

	InputChannel(int bufferSize);
	~InputChannel();

	/* [Channel] inheritance */
	void copy(const Channel* src, pthread_mutex_t* pluginMutex) override;
	void readPatch(const std::string& basePath, int i) override;
	void writePatch(bool isProject) override;
	void process(giada::m::AudioBuffer& out, giada::m::AudioBuffer& in) override;
	void parseAction(giada::m::recorder::action* a, int localFrame, int globalFrame, bool mixerIsRunning) override;

	int				inputIndex;
	giada::m::MidiDevice *midiInput;
};

#endif
