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
#include "const.h"

class ResourceChannel;
class geColumn;

class ColumnChannel : public Channel
{
private:

	/* ResourceChannel stack */
	std::vector <ResourceChannel*> resources;

	/* rChan
	Buffer for outputs of ResourceChannels. */

	giada::m::AudioBuffer rChan;

public:

	ColumnChannel(int bufferSize);
	~ColumnChannel();

	/* [Channel] inheritance */
	void copy(const Channel* src, pthread_mutex_t* pluginMutex) override;
	void readPatch(const std::string& basePath, int i) override;
	void writePatch(bool isProject) override;
	bool allocBuffers() override;
	void clearBuffers() override;
	void process(giada::m::AudioBuffer& out, giada::m::AudioBuffer& in) override;
	void parseAction(giada::m::recorder::action* a, int localFrame, int globalFrame, bool mixerIsRunning) override;
	void setMono(bool mono) override;


	/* */

	ResourceChannel* 	getResource(int index);
	void 			 				addResource(ResourceChannel* resource);
	void 			 				removeResource(int index);
	unsigned 		 			getResourceCount();

	/* ResourceChannel Stack methods */

	void recArmedResources();
	void stopRecResources();
	void clearAllResources();
	bool isSilent();

	/* Output */
	int	outputIndex;

	/* iterator */
	explicit ColumnChannel(std::initializer_list<ResourceChannel*> init) : Channel(G_CHANNEL_COLUMN, bufferSize, false), resources(init) {}

  using iterator = std::vector<ResourceChannel*>::iterator;
  using const_iterator = std::vector<ResourceChannel*>::const_iterator;

  iterator begin() { return resources.begin(); }
  iterator end() { return resources.end(); }
	const_iterator begin() const { return resources.begin(); }
  const_iterator end() const { return resources.end(); }
  const_iterator cbegin() const { return resources.cbegin(); }
  const_iterator cend() const { return resources.cend(); }
};


#endif
