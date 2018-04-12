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

#include "resourceChannel.h"

#include <cassert>
#include <cstring>
#include "../utils/log.h"
#include "../gui/elems/mainWindow/keyboard/channel.h"
#include "const.h"
#include "channel.h"
#include "sampleChannel.h"
#include "pluginHost.h"
#include "plugin.h"
#include "kernelMidi.h"
#include "patch.h"
#include "clock.h"
#include "wave.h"
#include "mixer.h"
#include "mixerHandler.h"
#include "conf.h"
#include "patch.h"
#include "waveFx.h"
#include "midiMapConf.h"


using std::string;
using namespace giada::m;


ResourceChannel::ResourceChannel(int type, int status, int bufferSize)
: Channel						(type, bufferSize),
	previewMode    		(G_PREVIEW_NONE),
	status         		(status),
	recStatus      		(REC_STOPPED),
	armed            	(false),
	column		 				(nullptr),
	midiInKeyPress 		(0x0),
	midiInKeyRel   		(0x0),
	midiInKill     		(0x0),
	midiInArm      		(0x0),
	midiOutLplaying		(0x0),
	midiInVeloAsVol		(true),
	midiInPitch      	(0x0)
{
}


/* -------------------------------------------------------------------------- */


ResourceChannel::~ResourceChannel()
{
	status = STATUS_OFF;
}

/* -------------------------------------------------------------------------- */


/*int previewMode;
float volume_i; // internal volume
float volume_d; // delta volume (for envelope)
int type;                  // midi or sample
int status;                // status: see const.h
int recStatus;             // status of recordings (waiting, ending, ...)
uint32_t midiInKeyPress;
uint32_t midiInKeyRel;
uint32_t midiInKill;
uint32_t midiInArm;
uint32_t midiOutLplaying;*/


void ResourceChannel::copy(const Channel *src, pthread_mutex_t *pluginMutex)
{
	// TODO - fix this (and all new Channel classes patches and copy)
}


/* -------------------------------------------------------------------------- */


void ResourceChannel::writePatch(bool isProject)
{
	/*
		TODO
	*/
}


/* -------------------------------------------------------------------------- */


void ResourceChannel::readPatch(const std::string& basePath, int i)
{
	/*
		TODO
	*/
}


/* -------------------------------------------------------------------------- */

bool ResourceChannel::isPlaying()
{
	return status & (STATUS_PLAY | STATUS_ENDING);
}

/* -------------------------------------------------------------------------- */

bool ResourceChannel::isRecording()
{
	return recStatus == REC_READING;
}

/* -------------------------------------------------------------------------- */


void ResourceChannel::sendMidiLplay()
{
	if (!midiOutL || midiOutLplaying == 0x0)
		return;
	switch (status) {
		case STATUS_OFF:
			sendMidiLmessage(midiOutLplaying, midimap::stopped);
			break;
		case STATUS_PLAY:
			sendMidiLmessage(midiOutLplaying, midimap::playing);
			break;
		case STATUS_WAIT:
			sendMidiLmessage(midiOutLplaying, midimap::waiting);
			break;
		case STATUS_ENDING:
			sendMidiLmessage(midiOutLplaying, midimap::stopping);
	}
}

void ResourceChannel::setVolumeI(float v)
{
	volume_i = v;
}

/* -------------------------------------------------------------------------- */


void ResourceChannel::setPreviewMode(int m)
{
	previewMode = m;
}


bool ResourceChannel::isPreview()
{
	return previewMode != G_PREVIEW_NONE;
}

/* -------------------------------------------------------------------------- */


void ResourceChannel::setReadActions(bool v, bool killOnFalse)
{
	readActions = v;
	if (!readActions && killOnFalse)
		kill(0);  /// FIXME - wrong frame value
}

/* -------------------------------------------------------------------------- */

void ResourceChannel::setOnPreviewEndCb(std::function<void()> f)
{
	onPreviewEnd = f;
}

/* -------------------------------------------------------------------------- */

void ResourceChannel::sendMidiLmute(){}
void ResourceChannel::sendMidiLsolo(){}
