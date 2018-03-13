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


#ifdef WITH_VST


#include "../utils/log.h"
#include "../utils/fs.h"
#include "../utils/string.h"
#include "const.h"
#include "channel.h"
#include "plugin.h"
#include "pluginHost.h"


using std::vector;
using std::string;


namespace giada {
namespace m {
namespace pluginHost
{
namespace
{
juce::MessageManager* messageManager;

/* pluginFormat
 * Plugin format manager. */

juce::VSTPluginFormat pluginFormat;

/* knownPuginList
 * List of known (i.e. scanned) plugins. */

juce::KnownPluginList knownPluginList;

/* unknownPluginList
 * List of unrecognized plugins found in a patch. */

vector<string> unknownPluginList;

vector<Plugin*> masterOut;
vector<Plugin*> masterIn;

/* Audio|MidiBuffer
 * Dynamic buffers. */

juce::AudioBuffer<float> audioBuffer;

int samplerate;
int buffersize;

/* missingPlugins
 * If some plugins from any stack are missing. */

bool missingPlugins;

void splitPluginDescription(const string& descr, vector<string>& out)
{
	// input:  VST-mda-Ambience-18fae2d2-6d646141  string
	// output: [2-------------] [1-----] [0-----]  vector.size() == 3
	
	string chunk = "";
	int count = 2;
	for (int i=descr.length()-1; i >= 0; i--) {
		if (descr[i] == '-' && count != 0) {
			out.push_back(chunk);
			count--;
			chunk = "";
		}
		else
			chunk += descr[i];
	}
	out.push_back(chunk);
}


/* findPluginDescription
Browses the list of known plug-ins until plug-in with id == 'id' is found.
Unfortunately knownPluginList.getTypeForIdentifierString(id) doesn't work for
VSTs: their ID is based on the plug-in file location. E.g.:

	/home/vst/mdaAmbience.so      -> VST-mdaAmbience-18fae2d2-6d646141
	/home/vst-test/mdaAmbience.so -> VST-mdaAmbience-b328b2f6-6d646141

The following function simply drops the first hash code during comparison. */

const juce::PluginDescription* findPluginDescription(const string& id)
{
	vector<string> idParts;
	splitPluginDescription(id, idParts);

	for (const juce::PluginDescription* pd : knownPluginList) {
		vector<string> tmpIdParts;
		splitPluginDescription(pd->createIdentifierString().toStdString(), tmpIdParts);
		if (idParts[0] == tmpIdParts[0] && idParts[2] == tmpIdParts[2])
			return pd;
	}
	return nullptr;
}
}; // {anonymous}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


pthread_mutex_t mutex_midi;


/* -------------------------------------------------------------------------- */


void close()
{
	messageManager->deleteInstance();
}


/* -------------------------------------------------------------------------- */


void init(int _buffersize, int _samplerate)
{
	gu_log("[pluginHost::init] initialize with buffersize=%d, samplerate=%d\n",
		_buffersize, _samplerate);

	messageManager = juce::MessageManager::getInstance();
	audioBuffer.setSize(2, _buffersize);
	samplerate = _samplerate;
	buffersize = _buffersize;
	missingPlugins = false;
	//unknownPluginList.empty();
	loadList(gu_getHomePath() + G_SLASH + "plugins.xml");

	pthread_mutex_init(&mutex_midi, nullptr);
}


/* -------------------------------------------------------------------------- */


int scanDirs(const string& dirs, const std::function<void(float)>& cb)
{
	gu_log("[pluginHost::scanDir] requested directories: '%s'\n", dirs.c_str());
	gu_log("[pluginHost::scanDir] current plugins: %d\n", knownPluginList.getNumTypes());

	knownPluginList.clear();   // clear up previous plugins

	vector<string> dirVec;
	gu_split(dirs, ";", &dirVec);

	juce::VSTPluginFormat format;
	juce::FileSearchPath searchPath;
	for (const string& dir : dirVec)
		searchPath.add(juce::File(dir));

	juce::PluginDirectoryScanner scanner(knownPluginList, format, searchPath, 
		true, juce::File::nonexistent); // true: recursive

	juce::String name;
	while (scanner.scanNextFile(false, name)) {
		gu_log("[pluginHost::scanDir]   scanning '%s'\n", name.toRawUTF8());
		cb(scanner.getProgress());
	}

	gu_log("[pluginHost::scanDir] %d plugin(s) found\n", knownPluginList.getNumTypes());
	return knownPluginList.getNumTypes();
}


/* -------------------------------------------------------------------------- */


int saveList(const string& filepath)
{
	int out = knownPluginList.createXml()->writeToFile(juce::File(filepath), "");
	if (!out)
		gu_log("[pluginHost::saveList] unable to save plugin list to %s\n", filepath.c_str());
	return out;
}


/* -------------------------------------------------------------------------- */


int loadList(const string& filepath)
{
	juce::XmlElement* elem = juce::XmlDocument::parse(juce::File(filepath));
	if (elem) {
		knownPluginList.recreateFromXml(*elem);
		delete elem;
		return 1;
	}
	return 0;
}


/* -------------------------------------------------------------------------- */


Plugin* addPlugin(const string& fid, pthread_mutex_t* mutex, 
	Channel* ch)
{
	vector<Plugin*>* pStack = &ch->plugins;

	/* Initialize plugin. The default mode uses getTypeForIdentifierString, 
	falling back to  getTypeForFile (deprecated) for old patches (< 0.14.4). */

	const juce::PluginDescription* pd = findPluginDescription(fid);
	if (pd == nullptr) {
		gu_log("[pluginHost::addPlugin] no plugin found with fid=%s! Trying with "
			"deprecated mode...\n", fid.c_str());
		pd = knownPluginList.getTypeForFile(fid);
		if (pd == nullptr) {
			gu_log("[pluginHost::addPlugin] still nothing to do, returning unknown plugin\n");
			missingPlugins = true;
			unknownPluginList.push_back(fid);
			return nullptr;
		}
	}

	juce::AudioPluginInstance* pi = pluginFormat.createInstanceFromDescription(*pd, samplerate, buffersize);
	if (!pi) {
		gu_log("[pluginHost::addPlugin] unable to create instance with fid=%s!\n", fid.c_str());
		missingPlugins = true;
		return nullptr;
	}
	gu_log("[pluginHost::addPlugin] plugin instance with fid=%s created\n", fid.c_str());

	Plugin* p = new Plugin(pi, samplerate, buffersize);

	/* Try to inject the plugin as soon as possible. */

	while (true) {
		if (pthread_mutex_trylock(mutex) != 0)
			continue;
		pStack->push_back(p);
		pthread_mutex_unlock(mutex);
		break;
	}

	gu_log("[pluginHost::addPlugin] channel=%d, plugin id=%s loaded (%s), stack size=%d\n",
		ch->index, fid.c_str(), p->getName().c_str(), pStack->size());

	return p;
}


/* -------------------------------------------------------------------------- */


Plugin* addPlugin(int index, pthread_mutex_t* mutex,
	Channel* ch)
{
	juce::PluginDescription* pd = knownPluginList.getType(index);
	if (pd) {
		gu_log("[pluginHost::addPlugin] plugin found, uid=%s, name=%s...\n",
			pd->createIdentifierString().toRawUTF8(), pd->name.toRawUTF8());
		return addPlugin(pd->createIdentifierString().toStdString(), mutex, ch);
	}
	gu_log("[pluginHost::addPlugin] no plugins found at index=%d!\n", index);
	return nullptr;
}


/* -------------------------------------------------------------------------- */


unsigned countPlugins(Channel* ch)
{
	return ch->plugins.size();
}


/* -------------------------------------------------------------------------- */


int countAvailablePlugins()
{
	return knownPluginList.getNumTypes();
}


/* -------------------------------------------------------------------------- */


unsigned countUnknownPlugins()
{
	return unknownPluginList.size();
}


/* -------------------------------------------------------------------------- */


pluginHost::PluginInfo getAvailablePluginInfo(int i)
{
	juce::PluginDescription* pd = knownPluginList.getType(i);
	PluginInfo pi;
	pi.uid = pd->fileOrIdentifier.toStdString();
	pi.name = pd->name.toStdString();
	pi.category = pd->category.toStdString();
	pi.manufacturerName = pd->manufacturerName.toStdString();
	pi.format = pd->pluginFormatName.toStdString();
	pi.isInstrument = pd->isInstrument;
	return pi;
}


/* -------------------------------------------------------------------------- */


bool hasMissingPlugins()
{
	return missingPlugins;
};


/* -------------------------------------------------------------------------- */


string getUnknownPluginInfo(int i)
{
	return unknownPluginList.at(i);
}


/* -------------------------------------------------------------------------- */


void freeStack(pthread_mutex_t* mutex, Channel* ch)
{
	vector<Plugin*>* pStack = &ch->plugins;

	if (pStack->size() == 0)
		return;

	while (true) {
		if (pthread_mutex_trylock(mutex) != 0)
			continue;
		for (unsigned i=0; i<pStack->size(); i++)
			delete pStack->at(i);
		pStack->clear();
		pthread_mutex_unlock(mutex);
		break;
	}
	gu_log("[pluginHost::freeStack] channel=%d freed\n", ch->index);
}


/* -------------------------------------------------------------------------- */


void processStack(float* buffer, Channel* ch)
{
	vector<Plugin*>* pStack = &ch->plugins;

	/* Empty stack, stack not found or mixer not ready: do nothing. */

	if (pStack == nullptr || pStack->size() == 0)
		return;

	/* MIDI channels must not process the current buffer: give them an empty one. 
	Sample channels and Master in/out want audio data instead: let's convert the 
	internal buffer from Giada to Juce. */

	if (ch != nullptr && ch->type == CHANNEL_MIDI) 
		audioBuffer.clear();
	else
		for (int i=0; i<buffersize; i++) {
			audioBuffer.setSample(0, i, buffer[i*2]);
			audioBuffer.setSample(1, i, buffer[(i*2)+1]);
		}

	/* Hardcore processing. At the end we swap input and output, so that he N-th
	plugin will process the result of the plugin N-1. Part of this loop must be
	guarded by mutexes, i.e. the MIDI process part. You definitely don't want
	a situation like the following one:
		this::processStack()
		[a new midi event comes in from kernelMidi thread]
		channel::clearMidiBuffer()
	The midi event in between would be surely lost, deleted by the last call to
	channel::clearMidiBuffer()! */

	if (ch != nullptr)
		pthread_mutex_lock(&mutex_midi);

	for (const Plugin* plugin : *pStack) {
		if (plugin->isSuspended() || plugin->isBypassed())
			continue;

		/* If this is a Channel (ch != nullptr) and the current plugin is an 
		instrument (i.e. accepts MIDI), don't let it fill the current audio buffer: 
		create a new temporary one instead and then merge the result into the main
		one when done. This way each plug-in generates its own audio data and we can
		play more than one plug-in instrument in the same stack, driven by the same
		set of MIDI events. */

		if (ch != nullptr && plugin->acceptsMidi()) {
			juce::AudioBuffer<float> tmp(2, buffersize);
			plugin->process(tmp, ch->getPluginMidiEvents());
			for (int i=0; i<buffersize; i++) {
				audioBuffer.addSample(0, i, tmp.getSample(0, i));
				audioBuffer.addSample(1, i, tmp.getSample(1, i));
			}
		}
		else
			plugin->process(audioBuffer, juce::MidiBuffer()); // Empty MIDI buffer
	}

	if (ch != nullptr) {
		ch->clearMidiBuffer();
		pthread_mutex_unlock(&mutex_midi);
	}

	/* Converting buffer from Juce to Giada. A note for the future: if we 
	overwrite (=) (as we do now) it's SEND, if we add (+) it's INSERT. */

	for (int i=0; i<buffersize; i++) {
		buffer[i*2]     = audioBuffer.getSample(0, i);
		buffer[(i*2)+1] = audioBuffer.getSample(1, i);
	}
}


/* -------------------------------------------------------------------------- */


Plugin* getPluginByIndex(int index, Channel* ch)
{
	vector<Plugin*>* pStack = &ch->plugins;
	if (pStack->size() == 0)
		return nullptr;
	if ((unsigned) index >= pStack->size())
		return nullptr;
	return pStack->at(index);
}


/* -------------------------------------------------------------------------- */


int getPluginIndex(int id, Channel* ch)
{
	vector<Plugin*>* pStack = &ch->plugins;
	for (unsigned i=0; i<pStack->size(); i++)
		if (pStack->at(i)->getId() == id)
			return i;
	return -1;
}


/* -------------------------------------------------------------------------- */


void swapPlugin(unsigned indexA, unsigned indexB,
	pthread_mutex_t* mutex, Channel* ch)
{
	vector<Plugin*>* pStack = &ch->plugins;
	while (true) {
		if (pthread_mutex_trylock(mutex) != 0)
			continue;
		std::swap(pStack->at(indexA), pStack->at(indexB));
		pthread_mutex_unlock(mutex);
		gu_log("[pluginHost::swapPlugin] plugin at index %d and %d swapped\n", indexA, indexB);
		return;
	}
}


/* -------------------------------------------------------------------------- */


int freePlugin(int id, pthread_mutex_t* mutex, Channel* ch)
{
	vector<Plugin*>* pStack = &ch->plugins;
	for (unsigned i=0; i<pStack->size(); i++) {
		Plugin *pPlugin = pStack->at(i);
		if (pPlugin->getId() != id)
			continue;
		while (true) {
			if (pthread_mutex_trylock(mutex) != 0)
				continue;
			delete pPlugin;
			pStack->erase(pStack->begin() + i);
			pthread_mutex_unlock(mutex);
			gu_log("[pluginHost::freePlugin] plugin id=%d removed\n", id);
			return i;
		}
	}
	gu_log("[pluginHost::freePlugin] plugin id=%d not found\n", id);
	return -1;
}


/* -------------------------------------------------------------------------- */


void runDispatchLoop()
{
	messageManager->runDispatchLoopUntil(10);
	//gu_log("[pluginHost::runDispatchLoop] %d, hasStopMessageBeenSent=%d\n", r, messageManager->hasStopMessageBeenSent());
}


/* -------------------------------------------------------------------------- */


void freeAllStacks(vector<Channel*>* channels, pthread_mutex_t* mutex)
{
	for (unsigned i=0; i<channels->size(); i++)
		freeStack(mutex, channels->at(i));
	missingPlugins = false;
	unknownPluginList.clear();
}


/* -------------------------------------------------------------------------- */


int clonePlugin(Plugin* src, pthread_mutex_t* mutex,
	Channel* ch)
{
	Plugin* p = addPlugin(src->getUniqueId(), mutex, ch);
	if (!p) {
		gu_log("[pluginHost::clonePlugin] unable to add new plugin to stack!\n");
		return 0;
	}

	for (int k=0; k<src->getNumParameters(); k++)
		p->setParameter(k, src->getParameter(k));

	return 1;
}


/* -------------------------------------------------------------------------- */


bool doesPluginExist(const string& fid)
{
	return pluginFormat.doesPluginStillExist(*knownPluginList.getTypeForFile(fid));
}


/* -------------------------------------------------------------------------- */


void sortPlugins(int method)
{
	switch (method) {
		case sortMethod::NAME:
			knownPluginList.sort(juce::KnownPluginList::SortMethod::sortAlphabetically, true);
			break;
		case sortMethod::CATEGORY:
			knownPluginList.sort(juce::KnownPluginList::SortMethod::sortByCategory, true);
			break;
		case sortMethod::MANUFACTURER:
			knownPluginList.sort(juce::KnownPluginList::SortMethod::sortByManufacturer, true);
			break;
		case sortMethod::FORMAT:
			knownPluginList.sort(juce::KnownPluginList::SortMethod::sortByFormat, true);
			break;
	}
}

}}}; // giada::m::pluginHost::


#endif // #ifdef WITH_VST
