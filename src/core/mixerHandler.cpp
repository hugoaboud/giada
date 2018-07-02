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


#include <vector>
#include <algorithm>
#include "../utils/fs.h"
#include "../utils/string.h"
#include "../utils/log.h"
#include "../glue/main.h"
#include "../glue/channel.h"
#include "kernelMidi.h"
#include "mixer.h"
#include "metronome.h"
#include "const.h"
#include "init.h"
#include "pluginHost.h"
#include "plugin.h"
#include "waveFx.h"
#include "conf.h"
#include "patch.h"
#include "recorder.h"
#include "clock.h"
#include "channel.h"
#include "kernelAudio.h"
#include "midiMapConf.h"
#include "inputChannel.h"
#include "resourceChannel.h"
#include "sampleChannel.h"
#include "columnChannel.h"
#include "midiChannel.h"
#include "wave.h"
#include "waveManager.h"
#include "channelManager.h"
#include "mixerHandler.h"


using std::vector;
using std::string;


namespace giada {
namespace m {
namespace mh
{
namespace
{
#ifdef WITH_VST

int readPatchPlugins(vector<patch::plugin_t>* list)
{
	int ret = 1;
	for (unsigned i=0; i<list->size(); i++) {
		patch::plugin_t *ppl = &list->at(i);
		// TODO use glue_addPlugin()
		Plugin *plugin = pluginHost::addPlugin(ppl->path.c_str(),
				&mixer::mutex_plugins, nullptr);
		if (plugin != nullptr) {
			plugin->setBypass(ppl->bypass);
			for (unsigned j=0; j<ppl->params.size(); j++)
				plugin->setParameter(j, ppl->params.at(j));
			ret &= 1;
		}
		else
			ret &= 0;
	}
	return ret;
}

#endif

/* ------------------------------------------------------------------------ */

int channelIndex = 0;

int getNewChannelIndex()
{
	return channelIndex++;
}


}; // {anonymous}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


bool uniqueSamplePath(const SampleChannel* skip, const string& path)
{
	for (ColumnChannel* cch : mixer::columnChannels) {
		for (ResourceChannel* ch : (*cch)) {
			if (skip == ch || ch->type != G_CHANNEL_SAMPLE) // skip itself and MIDI channels
				continue;
			const SampleChannel* sch = static_cast<const SampleChannel*>(ch);
			if (sch->wave != nullptr && path == sch->wave->getPath())
				return false;
		}
	}
	return true;
}


/* -------------------------------------------------------------------------- */


InputChannel* addInputChannel()
{
	InputChannel* ch;
	int bufferSize = kernelAudio::getRealBufSize();

	ch = new InputChannel(bufferSize);

	if (!ch->allocBuffers()) {
		delete ch;
		return nullptr;
	}

	while (true) {
		if (pthread_mutex_trylock(&mixer::mutex_chans) != 0)
			continue;
		mixer::inputChannels.push_back(ch);
		pthread_mutex_unlock(&mixer::mutex_chans);
		break;
	}

	ch->index = getNewChannelIndex();
	gu_log("[addInputChannel] channel index=%d added, total=%d\n", ch->index, mixer::inputChannels.size());
	return ch;
}



bool deleteInputChannel(InputChannel* ch) {
	int index = -1;
	for (unsigned i=0; i<mixer::inputChannels.size(); i++) {
		if (mixer::inputChannels.at(i) == ch) {
			index = i;
			break;
		}
	}
	if (index == -1) {
		gu_log("[deleteInputChannel] unable to find channel %d for deletion!\n", ch->index);
		return false;
	}

	while (true) {
		if (pthread_mutex_trylock(&mixer::mutex_chans) != 0)
			continue;
		mixer::inputChannels.erase(mixer::inputChannels.begin() + index);
		delete ch;
		pthread_mutex_unlock(&mixer::mutex_chans);
		return true;
	}
}

/* -------------------------------------------------------------------------- */


InputChannel* getInputChannelByIndex(int index)
{
	for (unsigned i=0; i<mixer::inputChannels.size(); i++)
		if (mixer::inputChannels.at(i)->index == index)
			return mixer::inputChannels.at(i);
	gu_log("[getInputChannelByIndex] input channel at index %d not found!\n", index);
	return nullptr;
}

/* -------------------------------------------------------------------------- */

ColumnChannel* addColumnChannel() {
	ColumnChannel* ch;
	int bufferSize = kernelAudio::getRealBufSize();

	ch = new ColumnChannel(bufferSize);

	if (!ch->allocBuffers()) {
		delete ch;
		return nullptr;
	}

	while (true) {
		if (pthread_mutex_trylock(&mixer::mutex_chans) != 0)
			continue;
		mixer::columnChannels.push_back(ch);
		pthread_mutex_unlock(&mixer::mutex_chans);
		break;
	}

	ch->index = getNewChannelIndex();
	ch->setName(("Column " + std::to_string(ch->index)).c_str());
	gu_log("[addColumnChannel] column channel index=%d added, total=%d\n", ch->index, mixer::columnChannels.size());
	return ch;
}

/* -------------------------------------------------------------------------- */

bool deleteColumnChannel(ColumnChannel* ch) {
	int index = -1;
	for (unsigned i=0; i<mixer::columnChannels.size(); i++) {
		if (mixer::columnChannels.at(i) == ch) {
			index = i;
			break;
		}
	}
	if (index == -1) {
		gu_log("[deleteColumnChannel] unable to find channel %d for deletion!\n", ch->index);
		return false;
	}

	while (true) {
		if (pthread_mutex_trylock(&mixer::mutex_chans) != 0)
			continue;
		mixer::columnChannels.erase(mixer::columnChannels.begin() + index);
		delete ch;
		pthread_mutex_unlock(&mixer::mutex_chans);
		return true;
	}
}

/* -------------------------------------------------------------------------- */

ColumnChannel* getColumnChannelByIndex(int index) {
	for (unsigned i=0; i<mixer::columnChannels.size(); i++)
		if (mixer::columnChannels.at(i)->index == index)
			return mixer::columnChannels.at(i);
	gu_log("[getColumnChannelByIndex] channel at index %d not found!\n", index);
	return nullptr;
}

/* -------------------------------------------------------------------------- */

ResourceChannel* addResourceChannel(ColumnChannel* col, int type)
{
	ResourceChannel* ch;
	int bufferSize = kernelAudio::getRealBufSize();

	if (type == G_CHANNEL_SAMPLE)
		ch = new SampleChannel(bufferSize);
	else
		ch = new MidiChannel(bufferSize);

	if (!ch->allocBuffers()) {
		delete ch;
		return nullptr;
	}

	while (true) {
		if (pthread_mutex_trylock(&mixer::mutex_chans) != 0)
			continue;
		col->addResource(ch);
		pthread_mutex_unlock(&mixer::mutex_chans);
		break;
	}

	ch->index = getNewChannelIndex();
	ch->column = col;
	gu_log("[addResourceChannel] channel index=%d added, total on column=%d\n", ch->index, col->getResourceCount());
	return ch;
}


/* -------------------------------------------------------------------------- */


bool deleteResourceChannel(ResourceChannel* ch)
{
	int columnIndex = -1;
	int index = -1;
	for (unsigned i=0; i<mixer::columnChannels.size(); i++) {
		ColumnChannel* column = mixer::columnChannels.at(i);
		for (unsigned j=0; j<column->getResourceCount(); j++) {
			if (column->getResource(i) == ch) {
				columnIndex = i;
				index = j;
				break;
			}
		}
	}
	if (columnIndex == -1 || index == -1) {
		gu_log("[deleteResourceChannel] unable to find channel %d for deletion!\n", ch->index);
		return false;
	}

	while (true) {
		if (pthread_mutex_trylock(&mixer::mutex_chans) != 0)
			continue;
		mixer::columnChannels.at(columnIndex)->removeResource(index);
		delete ch;
		pthread_mutex_unlock(&mixer::mutex_chans);
		return true;
	}
}


/* -------------------------------------------------------------------------- */


ResourceChannel* getResourceChannelByIndex(int index)
{
	for (unsigned i=0; i<mixer::columnChannels.size(); i++) {
		ColumnChannel* cch = mixer::columnChannels.at(i);
		for (unsigned j=0; j<cch->getResourceCount(); j++) {
			if (cch->getResource(j)->index == index)
				return cch->getResource(j);
		}
	}
	gu_log("[getResourceChannelByIndex] channel at index %d not found!\n", index);
	return nullptr;
}

/* -------------------------------------------------------------------------- */

Channel* getChannelByIndex(int index)
{
	InputChannel* input = getInputChannelByIndex(index);
	if (input != nullptr) return (Channel*) input;

	ColumnChannel* column = getColumnChannelByIndex(index);
	if (column != nullptr) return (Channel*) column;

	ResourceChannel* resource = getResourceChannelByIndex(index);
	if (resource != nullptr) return (Channel*) resource;

	gu_log("[getChannelByIndex] channel at index %d not found!\n", index);
	return nullptr;
}

/* -------------------------------------------------------------------------- */


bool hasLogicalSamples()
{
	// TODO: Fix this (call from columnchannels)
	/*for (unsigned i=0; i<mixer::channels.size(); i++) {
		SampleChannel *sch = static_cast<SampleChannel*>(mixer::channels.at(i));
		if (sch != nullptr && sch->wave && sch->wave->isLogical())
			return true;
	}*/
	return false;
}


/* -------------------------------------------------------------------------- */


bool hasEditedSamples()
{
	// TODO: Fix this (call from columnchannels)
	/*for (unsigned i=0; i<mixer::channels.size(); i++)
	{
		SampleChannel *sch = static_cast<SampleChannel*>(mixer::channels.at(i));
		if (sch != nullptr && sch->wave && sch->wave->isEdited())
			return true;
	}*/
	return false;
}


/* -------------------------------------------------------------------------- */


void stopSequencer()
{
	// TODO: Fix this (call from columnchannels)
	clock::stop();
	//for (unsigned i=0; i<mixer::channels.size(); i++)
	//	mixer::channels.at(i)->stopBySeq(conf::chansStopOnSeqHalt);
}


/* -------------------------------------------------------------------------- */


void updateSoloCount()
{
	for (ColumnChannel* cch : mixer::columnChannels)
		for (ResourceChannel* ch : (*cch))
			if (ch->solo) {
				mixer::hasSolos = true;
				return;
			}
	mixer::hasSolos = false;
}


/* -------------------------------------------------------------------------- */


void readPatch()
{
	mixer::ready = false;

	mixer::outVol = patch::masterVolOut;
	mixer::inVol = patch::masterVolIn;
	clock::setBpm(patch::bpm);
	clock::setBars(patch::bars);
	clock::setBeats(patch::beats);
	clock::setQuantize(patch::quantize);
	clock::updateFrameBars();
	metronome::on = patch::metronome;

#ifdef WITH_VST

	//readPatchPlugins(&patch::masterInPlugins, pluginHost::MASTER_IN);
	//readPatchPlugins(&patch::masterOutPlugins, pluginHost::MASTER_OUT);

#endif

	/* Rewind and update frames in Mixer. Also alloc new space in the virtual
	input buffer, in case the patch has a sequencer size != default one (which is
	very likely). */

	mixer::rewind();
	mixer::ready = true;
}


/* -------------------------------------------------------------------------- */


void rewindSequencer()
{
	if (clock::getQuantize() > 0 && clock::isRunning())   // quantize rewind
		mixer::rewindWait = true;
	else
		mixer::rewind();
}


/* -------------------------------------------------------------------------- */


void startInputRec()
{
	gu_log("[mh] start input rec\n");
	mixer::recording = true;

	for (unsigned i=0; i<mixer::columnChannels.size(); i++) {
		mixer::columnChannels[i]->recArmedResources();
	}
}


/* -------------------------------------------------------------------------- */


void stopInputRec()
{
	gu_log("[mh] stop input rec\n");
	mixer::recording = false;

	for (unsigned i=0; i<mixer::columnChannels.size(); i++) {
		mixer::columnChannels[i]->stopRecResources();
	}
}


/* -------------------------------------------------------------------------- */


bool hasArmedSampleChannels()
{
	// TODO: delete this method
	/*for (unsigned i=0; i<mixer::channels.size(); i++) {
		Channel *ch = mixer::channels.at(i);
		if (ch->type == CHANNEL_SAMPLE && ch->isArmed())
			return true;
	}*/
	return false;
}


}}}; // giada::m::mh::
