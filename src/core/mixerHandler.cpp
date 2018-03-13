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
		if (skip == ch || ch->type != CHANNEL_SAMPLE) // skip itself and MIDI channels
			continue;
		const SampleChannel* sch = static_cast<const SampleChannel*>(ch);
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
	gu_log("[addChannel] channel index=%d added, type=%d, total=%d\n",
		ch->index, ch->type, mixer::channels.size());
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
	gu_log("[addChannel] column channel index=%d added, type=%d, total=%d\n",
		ch->index, ch->type, mixer::channels.size());
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

Channel* addChannel(int type)
{
	Channel* ch;
	int bufferSize = kernelAudio::getRealBufSize()*2;

	if (type == CHANNEL_SAMPLE)
		ch = new SampleChannel(bufferSize, conf::inputMonitorDefaultOn);
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
	gu_log("[addChannel] channel index=%d added, type=%d, total=%d\n",
		ch->index, ch->type, mixer::channels.size());
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


Channel* getChannelByIndex(int index)
{
	for (unsigned i=0; i<mixer::channels.size(); i++)
		if (mixer::channels.at(i)->index == index)
			return mixer::channels.at(i);
	gu_log("[getChannelByIndex] channel at index %d not found!\n", index);
	return nullptr;
}


/* -------------------------------------------------------------------------- */


bool hasLogicalSamples()
{
	for (unsigned i=0; i<mixer::channels.size(); i++) {
		if (mixer::channels.at(i)->type != CHANNEL_SAMPLE)
			continue;
		SampleChannel *ch = static_cast<SampleChannel*>(mixer::channels.at(i));
		if (ch->wave && ch->wave->isLogical())
			return true;
	}
	return false;
}


/* -------------------------------------------------------------------------- */


bool hasEditedSamples()
{
	for (unsigned i=0; i<mixer::channels.size(); i++)
	{
		if (mixer::channels.at(i)->type != CHANNEL_SAMPLE)
			continue;
		SampleChannel *ch = static_cast<SampleChannel*>(mixer::channels.at(i));
		if (ch->wave && ch->wave->isEdited())
			return true;
	}
	return false;
}


/* -------------------------------------------------------------------------- */


void stopSequencer()
{
	clock::stop();
	for (unsigned i=0; i<mixer::channels.size(); i++)
		mixer::channels.at(i)->stopBySeq(conf::chansStopOnSeqHalt);
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


bool startInputRec()
{
	int channelsReady = 0;

	for (Channel* channel : mixer::channels) {

		if (!channel->canInputRec())
			continue;

		SampleChannel* ch = static_cast<SampleChannel*>(channel);

		/* Allocate empty sample for the current channel. */

		Wave* wave = nullptr;
		int result = waveManager::createEmpty(clock::getTotalFrames(), 
			conf::samplerate, string("TAKE-" + gu_iToString(patch::lastTakeId)), &wave); 
		if (result != G_RES_OK) {
			gu_log("[startInputRec] unable to allocate new Wave in chan %d!\n",
				ch->index);
			continue;
		}

		ch->pushWave(wave);
		ch->setName("TAKE-" + gu_iToString(patch::lastTakeId++)); // Increase lastTakeId 
		channelsReady++;

		gu_log("[startInputRec] start input recs using chan %d with size %d "
			"frame=%d\n", ch->index, clock::getTotalFrames(), mixer::inputTracker);
	}

	if (channelsReady > 0) {
		mixer::recording = true;
		/* start to write from the currentFrame, not the beginning */
		/** FIXME: this should be done before wave allocation */
		mixer::inputTracker = clock::getCurrentFrame();
		return true;
	}
	return false;
}


/* -------------------------------------------------------------------------- */


void stopInputRec()
{
	mixer::mergeVirtualInput();
	mixer::recording = false;
	mixer::waitRec = 0; // in case delay compensation is in use
	gu_log("[mh] stop input recs\n");
}


/* -------------------------------------------------------------------------- */


bool hasArmedSampleChannels()
{
	for (unsigned i=0; i<mixer::channels.size(); i++) {
		Channel *ch = mixer::channels.at(i);
		if (ch->type == CHANNEL_SAMPLE && ch->isArmed())
			return true;
	}
	return false;
}


}}}; // giada::m::mh::
