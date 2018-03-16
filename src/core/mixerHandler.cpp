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
#include "../utils/fs.h"
#include "../utils/string.h"
#include "../utils/log.h"
#include "../glue/main.h"
#include "../glue/channel.h"
#include "kernelMidi.h"
#include "mixer.h"
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

int getNewInputChanIndex()
{
	/* always skip last channel: it's the last one just added */

	if (mixer::inputChannels.size() == 1)
		return 0;

	int index = 0;
	for (unsigned i=0; i<mixer::inputChannels.size()-1; i++) {
		if (mixer::inputChannels.at(i)->index > index)
			index = mixer::inputChannels.at(i)->index;
		}
	index += 1;
	return index;
}

/* ------------------------------------------------------------------------ */

int getNewColumnChanIndex()
{
	/* always skip last channel: it's the last one just added */

	if (mixer::columnChannels.size() == 1)
		return 0;

	int index = 0;
	for (unsigned i=0; i<mixer::columnChannels.size()-1; i++) {
		if (mixer::columnChannels.at(i)->index > index)
			index = mixer::columnChannels.at(i)->index;
		}
	index += 1;
	return index;
}

/* ------------------------------------------------------------------------ */


int getNewChanIndex()
{
	/* always skip last channel: it's the last one just added */

	if (mixer::channels.size() == 1)
		return 0;

	int index = 0;
	for (unsigned i=0; i<mixer::channels.size()-1; i++) {
		if (mixer::channels.at(i)->index > index)
			index = mixer::channels.at(i)->index;
		}
	index += 1;
	return index;
}


}; // {anonymous}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


bool uniqueSamplePath(const SampleChannel* skip, const string& path)
{
	for (const Channel* ch : mixer::channels) {
		const SampleChannel* sch = static_cast<const SampleChannel*>(ch);
		if (skip == ch || (sch != nullptr)) // skip itself and MIDI channels
			continue;
		if (sch->wave != nullptr && path == sch->wave->getPath())
			return false;
	}
	return true;
}


/* -------------------------------------------------------------------------- */


InputChannel* addInputChannel()
{
	InputChannel* ch;
	int bufferSize = kernelAudio::getRealBufSize()*2;

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

	ch->index = getNewInputChanIndex();
	gu_log("[addChannel] channel index=%d added, total=%d\n", ch->index, mixer::channels.size());
	return ch;
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
	int bufferSize = kernelAudio::getRealBufSize()*2;

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

	ch->index = getNewColumnChanIndex();
	ch->setName(("Column " + std::to_string(ch->index)).c_str());
	gu_log("[addChannel] column channel index=%d added, total=%d\n", ch->index, mixer::channels.size());
	return ch;
}

/* -------------------------------------------------------------------------- */

int deleteColumnChannel(ColumnChannel* ch) {
	int index = -1;
	for (unsigned i=0; i<mixer::columnChannels.size(); i++) {
		if (mixer::columnChannels.at(i) == ch) {
			index = i;
			break;
		}
	}
	if (index == -1) {
		gu_log("[deleteChannel] unable to find channel %d for deletion!\n", ch->index);
		return 0;
	}

	while (true) {
		if (pthread_mutex_trylock(&mixer::mutex_chans) != 0)
			continue;
		mixer::columnChannels.erase(mixer::columnChannels.begin() + index);
		delete ch;
		pthread_mutex_unlock(&mixer::mutex_chans);
		return 1;
	}
}

/* -------------------------------------------------------------------------- */

ColumnChannel* getColumnChannelByIndex(int index) {
	for (unsigned i=0; i<mixer::columnChannels.size(); i++)
		if (mixer::columnChannels.at(i)->index == index)
			return mixer::columnChannels.at(i);
	gu_log("[getInputChannelByIndex] channel at index %d not found!\n", index);
	return nullptr;
}

/* -------------------------------------------------------------------------- */

ResourceChannel* addChannel(int type)
{
	ResourceChannel* ch;
	int bufferSize = kernelAudio::getRealBufSize()*2;

	if (type == CHANNEL_SAMPLE)
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
		mixer::channels.push_back(ch);
		pthread_mutex_unlock(&mixer::mutex_chans);
		break;
	}

	ch->index = getNewChanIndex();
	gu_log("[addChannel] channel index=%d added, total=%d\n", ch->index, mixer::channels.size());
	return ch;
}


/* -------------------------------------------------------------------------- */


int deleteChannel(Channel* ch)
{
	int index = -1;
	for (unsigned i=0; i<mixer::channels.size(); i++) {
		if (mixer::channels.at(i) == ch) {
			index = i;
			break;
		}
	}
	if (index == -1) {
		gu_log("[deleteChannel] unable to find channel %d for deletion!\n", ch->index);
		return 0;
	}

	while (true) {
		if (pthread_mutex_trylock(&mixer::mutex_chans) != 0)
			continue;
		mixer::channels.erase(mixer::channels.begin() + index);
		delete ch;
		pthread_mutex_unlock(&mixer::mutex_chans);
		return 1;
	}
}


/* -------------------------------------------------------------------------- */


ResourceChannel* getChannelByIndex(int index)
{
	for (unsigned i=0; i<mixer::columnChannels.size(); i++) {
		ColumnChannel* cch = mixer::columnChannels.at(i);
		for (unsigned j=0; j<cch->getResourceCount(); j++) {
			if (cch->getResource(j)->index == index)
				return cch->getResource(j);
		}
	}
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


bool uniqueSolo(Channel* ch)
{
	int solos = 0;
	for (unsigned i=0; i<mixer::channels.size(); i++) {
		Channel *ch = mixer::channels.at(i);
		if (ch->solo) solos++;
		if (solos > 1) return false;
	}
	return true;
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
	mixer::metronome = patch::metronome;

#ifdef WITH_VST

	//readPatchPlugins(&patch::masterInPlugins, pluginHost::MASTER_IN);
	//readPatchPlugins(&patch::masterOutPlugins, pluginHost::MASTER_OUT);

#endif

	/* Rewind and update frames in Mixer. Also alloc new space in the virtual
	input buffer, in case the patch has a sequencer size != default one (which is
	very likely). */

	mixer::rewind();
	mixer::allocVirtualInput(clock::getTotalFrames());
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
