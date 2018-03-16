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


#ifndef G_COLUMN_CHANNEL_H
#define G_COLUMN_CHANNEL_H

#include "channel.h"

class ResourceChannel;

class ColumnChannel : public Channel
{
private:

	/* ResourceChannel stack */
	std::vector <ResourceChannel*> resources;

public:

	ColumnChannel(int bufferSize);
	~ColumnChannel();

	/* Channel inherit*/

	std::string getName() const override;
	void process(float* outBuffer, float* inBuffer) override;
	void input(float* inBuffer) override;
	void copy(const Channel *src, pthread_mutex_t *pluginMutex) override;
	void parseAction(giada::m::recorder::action* a, int localFrame, int globalFrame, bool mixerIsRunning) override;
	bool isChainAlive() override;

	/* */

	ResourceChannel* getResource(int index);
	void 			 addResource(ResourceChannel* resource);
	unsigned 		 getResourceCount();

	/* ResourceChannel stack methods */

	void recArmedResources();
	void stopRecResources();
	void clearAllResources();
	bool isSilent();
};


#endif
