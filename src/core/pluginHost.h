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

#ifndef G_PLUGIN_HOST_H
#define G_PLUGIN_HOST_H


#include <functional>
#include <pthread.h>
#include "../deps/juce-config.h"
#include "audioBuffer.h"


class Plugin;
class Channel;


namespace giada {
namespace m {
namespace pluginHost
{

enum sortMethod
{
	NAME,
	CATEGORY,
	MANUFACTURER,
	FORMAT
};

struct PluginInfo
{
	std::string uid;
	std::string name;
	std::string category;
	std::string manufacturerName;
	std::string format;
	bool isInstrument;
};

extern pthread_mutex_t mutex_midi;

void init(int bufSize, int samplerate);
void close();

/* scanDirs
Parses plugin directories (semicolon-separated) and store list in
knownPluginList. The callback is called on each plugin found. Used to update the
main window from the GUI thread. */

int scanDirs(const std::string& paths, const std::function<void(float)>& cb);

/* (save|load)List
 * (Save|Load) knownPluginList (in|from) an XML file. */

int saveList(const std::string& path);
int loadList(const std::string& path);

/* addPlugin
 * Add a new plugin to stack by unique id or by index in knownPluginList
 * std::vector. Requires:
 * fid - plugin unique file id (i.e. path to dynamic library)
 * mutex - Mixer.mutex_plugin
 * freq - current audio frequency
 * bufSize - buffer size */

Plugin* addPlugin(const std::string& fid, pthread_mutex_t* mutex,
	Channel* ch=nullptr);
Plugin *addPlugin(int index, pthread_mutex_t* mutex,
	Channel* ch=nullptr);

/* countPlugins
 * Return size of channel's stack. */

unsigned countPlugins(Channel* ch=nullptr);

/* countAvailablePlugins
 * Return size of knownPluginList. */

int countAvailablePlugins();

/* countUnknownPlugins
 * Return size of unknownPluginList. */

unsigned countUnknownPlugins();

/* getAvailablePluginInfo
 * Return the available plugin information (name, type, ...) from
 * knownPluginList at index 'index'. */

PluginInfo getAvailablePluginInfo(int index);

std::string getUnknownPluginInfo(int index);

/* freeStack
 * free channel's plugin stack. */

void freeStack(pthread_mutex_t* mutex, Channel* ch=nullptr);

/* processStack
Applies the fx list to the buffer. */

void processStack(AudioBuffer& buffer, Channel* ch=nullptr);

/* getPluginByIndex */

Plugin* getPluginByIndex(int index, Channel* ch=nullptr);

/* getPluginIndex */

int getPluginIndex(int id, Channel* ch=nullptr);

/* swapPlugin */

void swapPlugin(unsigned indexA, unsigned indexB,
	pthread_mutex_t* mutex, Channel* ch=nullptr);

/* freePlugin.
Returns the internal stack index of the deleted plugin. */

int freePlugin(int id, pthread_mutex_t *mutex,
	Channel* ch=nullptr);

/* runDispatchLoop
 * Wakes up plugins' GUI manager for N milliseconds. */

void runDispatchLoop();

/* freeAllStacks
 * Frees everything. */

void freeAllStacks(std::vector<Channel*>* channels, pthread_mutex_t* mutex);

/* clonePlugin */

int clonePlugin(Plugin* src, pthread_mutex_t* mutex, Channel* ch);

/* doesPluginExist */

bool doesPluginExist(const std::string& fid);

bool hasMissingPlugins();

void sortPlugins(int sortMethod);

void forEachPlugin(int stackType, const Channel* ch, std::function<void(const Plugin* p)> f);

}}}; // giada::m::pluginHost::


#endif

#endif // #ifdef WITH_VST
