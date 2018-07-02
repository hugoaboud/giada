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


#ifndef G_GLUE_CHANNEL_H
#define G_GLUE_CHANNEL_H


#include <string>


class Channel;
class InputChannel;
class ColumnChannel;
class ResourceChannel;
class SampleChannel;
class gdSampleEditor;

namespace giada {
namespace c     {
namespace channel
{

/**
	ColumnChannel
**/

/* addColumnChannel
Adds an empty new channel to the stack. Returns the new channel. */

ColumnChannel* addColumnChannel(int width);

/* deletChannel
Removes a Channel from Mixer. */

void deleteChannel(Channel* ch, bool warn=true);

/**
	ResourceChannel
**/

/* addResourceChannel
Adds an empty new channel to the stack. Returns the new channel. */

ResourceChannel* addResourceChannel(ColumnChannel* col, int type, int size);

/* loadChannel
Fills an existing SampleChannel with a wave. */

int loadChannel(SampleChannel* ch, const std::string& fname);

/* freeResourceChannel
Unloads the sample from a sample channel. */

void freeResourceChannel(ResourceChannel* ch);

/* cloneResourceChannel
Makes an exact copy of Channel *ch. */

int cloneResourceChannel(ResourceChannel* ch);

/* toggle/set*
Toggles or set several channel properties. If gui == true the signal comes from
a manual interaction on the GUI, otherwise it's a MIDI/Jack/external signal. */

void setInput(ColumnChannel* ch, InputChannel* input);
void toggleArm(ResourceChannel* ch, bool gui);
void toggleInputMonitor(Channel* ch);
void setVolume(Channel* ch, float v, bool gui=true, bool editor=false);
void setPitch(SampleChannel* ch, float val);
void setPanning(ResourceChannel* ch, float val);
void setBoost(SampleChannel* ch, float val);
void setName(Channel* ch, const std::string& name);
void toggleMute(Channel* ch, bool gui=true);
void toggleSolo(Channel* ch, bool gui=true);
void setSolo(Channel* ch, bool v, bool gui);
void kill(ResourceChannel* ch);


/* toggleReadingRecs
Handles the 'R' button. If gui == true the signal comes from an user interaction
on the GUI, otherwise it's a MIDI/Jack/external signal. */

void toggleReadingRecs(ResourceChannel* ch, bool gui=true);
void startReadingRecs(ResourceChannel* ch, bool gui=true);
void stopReadingRecs(ResourceChannel* ch, bool gui=true);

}}}; // giada::c::channel::

#endif
