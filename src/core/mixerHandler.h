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


#ifndef G_MIXER_HANDLER_H
#define G_MIXER_HANDLER_H


#include <string>


class Channel;
class InputChannel;
class ColumnChannel;
class ResourceChannel;
class SampleChannel;


namespace giada {
namespace m {
namespace mh
{

/**
	INPUT
**/

/* addInputChannel
Adds a new input channel into mixer's stack. */

InputChannel* addInputChannel();

/* deleteInputChannel
Completely removes a column hannel from the stack. */

int deleteInputChannel(InputChannel* ch);

/* getInputChannelCount
Get how many inputs are on stack. */

unsigned getInputChannelCount();

/* getInputChannelByIndex
Returns channel with given index 'i'. */

InputChannel* getInputChannelByIndex(int i);

/**
	COLUMN
**/

/* addColumnChannel
Adds a new column channel into mixer's stack. */

ColumnChannel* addColumnChannel();

/* deleteChannel
Completely removes a column hannel from the stack. */

int deleteColumnChannel(ColumnChannel* ch);

/* getColumnChannelCount
Get how many inputs are on stack. */

unsigned getColumnChannelCount();

/* getColumnChannelByIndex
Returns column channel with given index 'i'. */

ColumnChannel* getColumnChannelByIndex(int i);

/**
**/

/* addChannel
Adds a new resource channel of type 'type' into mixer's stack. */

ResourceChannel* addResourceChannel(ColumnChannel* col, int type);

/* deleteChannel
Completely removes a resource channel from the stack. */

int deleteResourceChannel(ResourceChannel* ch);

/* getChannelByIndex
Returns channel with given index 'i'. */

ResourceChannel* getResourceChannelByIndex(int i);

/* hasLogicalSamples
True if 1 or more samples are logical (memory only, such as takes) 
TODO: fix this*/

bool hasLogicalSamples();

/* hasEditedSamples
True if 1 or more samples was edited via gEditor 
TODO: fix this*/

bool hasEditedSamples();

/* stopSequencer
Stops the sequencer, with special case if samplesStopOnSeqHalt is true. */

void stopSequencer();

void rewindSequencer();

/* uniqueSolo
 * true if ch is the only solo'd channel in mixer.
 TODO: fix this */

bool uniqueSolo(Channel* ch);

/* loadPatch
Loads a path or a project (if isProject) into Mixer. If isProject, path must 
contain the address of the project folder. */

void readPatch();

/* startInputRec - record from line in
Creates a new empty wave in the first available channels. Returns false if
something went wrong. */

void startInputRec();

void stopInputRec();

/* uniqueSamplePath
Returns true if path 'p' is unique. Requires SampleChannel 'skip' in order
to skip check against itself. */

bool uniqueSamplePath(const SampleChannel* skip, const std::string& p);

/* hasArmedSampleChannels
Tells whether Mixer has one or more sample channels armed for input
recording. 
TODO: fix this */

bool hasArmedSampleChannels();
}}}  // giada::m::mh::


#endif
