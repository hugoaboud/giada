 /* -----------------------------------------------------------------------------
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

#include <cmath>
#include <FL/Fl.H>
#include "../gui/dialogs/gd_mainWindow.h"
#include "../gui/dialogs/sampleEditor.h"
#include "../gui/dialogs/columnList.h"
#include "../gui/dialogs/inputList.h"
#include "../gui/dialogs/gd_warnings.h"
#include "../gui/elems/basics/input.h"
#include "../gui/elems/basics/dial.h"
#include "../gui/elems/sampleEditor/waveTools.h"
#include "../gui/elems/sampleEditor/volumeTool.h"
#include "../gui/elems/sampleEditor/boostTool.h"
#include "../gui/elems/sampleEditor/panTool.h"
#include "../gui/elems/sampleEditor/pitchTool.h"
#include "../gui/elems/sampleEditor/rangeTool.h"
#include "../gui/elems/sampleEditor/waveform.h"
#include "../gui/elems/mainWindow/keyboard/keyboard.h"
#include "../gui/elems/mainWindow/keyboard/channel.h"
#include "../gui/elems/mainWindow/keyboard/column.h"
#include "../gui/elems/mainWindow/keyboard/sampleChannel.h"
#include "../gui/elems/mainWindow/keyboard/channelButton.h"
#include "../utils/gui.h"
#include "../utils/fs.h"
#include "../utils/log.h"
#include "../core/kernelAudio.h"
#include "../core/mixerHandler.h"
#include "../core/mixer.h"
#include "../core/clock.h"
#include "../core/pluginHost.h"
#include "../core/conf.h"
#include "../core/wave.h"
#include "../core/channel.h"
#include "../core/columnChannel.h"
#include "../core/sampleChannel.h"
#include "../core/midiChannel.h"
#include "../core/plugin.h"
#include "../core/waveManager.h"
#include "main.h"
#include "channel.h"


extern gdMainWindow* G_MainWin;


using std::string;


namespace giada {
namespace c     {
namespace channel
{
int loadChannel(SampleChannel* ch, const string& fname)
{
	using namespace giada::m;

	/* Always stop a channel before loading a new sample in it. This will prevent
	issues if tracker is outside the boundaries of the new sample -> segfault. */

	if (ch->status & (STATUS_PLAY | STATUS_ENDING))
		ch->hardStop(0);

	/* Save the patch and take the last browser's dir in order to re-use it the
	next time. */

	conf::samplePath = gu_dirname(fname);

	Wave* wave = nullptr;
	int result = waveManager::create(fname, &wave);
	if (result != G_RES_OK)
		return result;

	if (wave->getRate() != conf::samplerate) {
		gu_log("[loadChannel] input rate (%d) != system rate (%d), conversion needed\n",
			wave->getRate(), conf::samplerate);
		result = waveManager::resample(wave, conf::rsmpQuality, conf::samplerate);
		if (result != G_RES_OK) {
			delete wave;
			return result;
		}
	}

	ch->pushWave(wave);

	G_MainWin->keyboard->updateChannel(ch->guiChannel);

	return result;
}

/* -------------------------------------------------------------------------- */

/* addColumnChannel
Adds an empty new channel to the stack. Returns the new channel. */

ColumnChannel* addColumnChannel(int width) {
  ColumnChannel* ch    = m::mh::addColumnChannel();
	geColumn* gch = G_MainWin->keyboard->addColumn(ch, width);
	ch->guiChannel = (geChannel*) gch;

	gdColumnList* gdColumn = static_cast<gdColumnList*>(gu_getSubwindow(G_MainWin, WID_COLUMN_LIST));
	if (gdColumn) {
		Fl::lock();
		gdColumn->refreshList();
		Fl::unlock();
	}
	gch->update();

	return ch;
}

/* deleteChannel
Removes a Channel from Mixer. */

void deleteChannel(Channel* ch, bool warn) {
	using namespace giada::m;

	if (warn) {
		switch (ch->type) {
			case G_CHANNEL_INPUT:
				if (!gdConfirmWin("Warning", "Delete input: are you sure?"))
					return;
				break;
			case G_CHANNEL_COLUMN:
				if (!gdConfirmWin("Warning", "Delete column: are you sure?"))
					return;
				break;
			case G_CHANNEL_SAMPLE:
			case G_CHANNEL_MIDI:
				if (!gdConfirmWin("Warning", "Delete channel: are you sure?"))
					return;
				break;
		}
	}

	recorder::clearChan(ch->index);
	ch->hasActions = false;
#ifdef WITH_VST
	pluginHost::freeStack(&mixer::mutex_plugins, ch);
#endif

	Fl::lock();
	if (ch->type == G_CHANNEL_SAMPLE || ch->type == G_CHANNEL_MIDI)
		G_MainWin->keyboard->deleteChannel(ch->guiChannel);
	else if (ch->type == G_CHANNEL_COLUMN)
		G_MainWin->keyboard->deleteColumn((geColumn*)ch->guiChannel);
	Fl::unlock();

	if (ch->type == G_CHANNEL_INPUT) {
			mh::deleteInputChannel((InputChannel*)ch);
			gdInputList* gdInput = static_cast<gdInputList*>(gu_getSubwindow(G_MainWin, WID_INPUT_LIST));
			if (gdInput) {
				Fl::lock();
				gdInput->refreshList();
				Fl::unlock();
			}
	}
	else if (ch->type == G_CHANNEL_COLUMN) {
			mh::deleteColumnChannel((ColumnChannel*)ch);
			gdColumnList* gdColumn = static_cast<gdColumnList*>(gu_getSubwindow(G_MainWin, WID_COLUMN_LIST));
			if (gdColumn) {
				Fl::lock();
				gdColumn->refreshList();
				Fl::unlock();
			}
	}
	else if (ch->type == G_CHANNEL_SAMPLE || ch->type == G_CHANNEL_MIDI) {
			mh::deleteResourceChannel((ResourceChannel*)ch);
	}

	gu_closeAllSubwindows();
}

/* -------------------------------------------------------------------------- */


ResourceChannel* addResourceChannel(ColumnChannel* col, int type, int size)
{
	ResourceChannel* ch    = m::mh::addResourceChannel(col, type);
	geChannel* gch = G_MainWin->keyboard->addChannel(col, ch, size);
	ch->guiChannel = gch;
	return ch;
}

/* -------------------------------------------------------------------------- */


void freeResourceChannel(ResourceChannel* ch)
{
	if (ch->status == STATUS_PLAY) {
		if (!gdConfirmWin("Warning", "This action will stop the channel: are you sure?"))
			return;
	}
	else
	if (!gdConfirmWin("Warning", "Free channel: are you sure?"))
		return;

	G_MainWin->keyboard->freeChannel(ch->guiChannel);
	m::recorder::clearChan(ch->index);
	ch->hasActions = false;
	ch->empty();

	/* delete any related subwindow */
	/** TODO - use gu_closeAllSubwindows()   */
	G_MainWin->delSubWindow(WID_FILE_BROWSER);
	G_MainWin->delSubWindow(WID_ACTION_EDITOR);
	G_MainWin->delSubWindow(WID_SAMPLE_EDITOR);
	G_MainWin->delSubWindow(WID_FX_LIST);
}

/* -------------------------------------------------------------------------- */


int cloneResourceChannel(ResourceChannel* src)
{
	using namespace giada::m;

	ResourceChannel* ch    = mh::addResourceChannel(src->column, src->getType());
	geChannel* gch = G_MainWin->keyboard->addChannel(src->column, ch, src->guiChannel->getSize());

	ch->guiChannel = gch;
	ch->copy(src, &mixer::mutex_plugins);

	G_MainWin->keyboard->updateChannel(ch->guiChannel);
	return true;
}

/* -------------------------------------------------------------------------- */


void toggleArm(ResourceChannel* ch, bool gui)
{
	ch->armed = !ch->armed;
	if (!gui)
		((geResourceChannel*)ch->guiChannel)->arm->value(ch->armed);
}


/* -------------------------------------------------------------------------- */


void toggleInputMonitor(Channel* ch)
{
	SampleChannel* sch = static_cast<SampleChannel*>(ch);
	sch->inputMonitor = !sch->inputMonitor;
}

/* -------------------------------------------------------------------------- */


void setVolume(Channel* ch, float v, bool gui, bool editor)
{
	ch->volume = v;

	/* Changing channel volume? Update wave editor (if it's shown). */

	if (!editor) {
		gdSampleEditor* gdEditor = static_cast<gdSampleEditor*>(gu_getSubwindow(G_MainWin, WID_SAMPLE_EDITOR));
		if (gdEditor) {
			Fl::lock();
			gdEditor->volumeTool->refresh();
			Fl::unlock();
		}
	}

	if (!gui) {
		Fl::lock();
		ch->guiChannel->vol->value(v);
		Fl::unlock();
	}
}


/* -------------------------------------------------------------------------- */


void setPitch(SampleChannel* ch, float val)
{
	ch->setPitch(val);
	gdSampleEditor* gdEditor = static_cast<gdSampleEditor*>(gu_getSubwindow(G_MainWin, WID_SAMPLE_EDITOR));
	if (gdEditor) {
		Fl::lock();
		gdEditor->pitchTool->refresh();
		Fl::unlock();
	}
}


/* -------------------------------------------------------------------------- */


void setPanning(ResourceChannel* ch, float val)
{
	ch->setPan(val);
	gdSampleEditor* gdEditor = static_cast<gdSampleEditor*>(gu_getSubwindow(G_MainWin, WID_SAMPLE_EDITOR));
	if (gdEditor) {
		Fl::lock();
		gdEditor->panTool->refresh();
		Fl::unlock();
	}
}


/* -------------------------------------------------------------------------- */


void toggleMute(Channel* ch, bool gui)
{
	using namespace giada::m;

	if (recorder::active && recorder::canRec(ch, clock::isRunning(), mixer::recording)) {
		if (!ch->mute) {
			recorder::startOverdub(ch->index, G_ACTION_MUTES, clock::getCurrentFrame(),
				kernelAudio::getRealBufSize());
			ch->setReadActions(false);   // don't read actions while overdubbing
		}
		else
		 recorder::stopOverdub(clock::getCurrentFrame(), clock::getFramesInLoop(),
			&mixer::mutex_recs);
	}

	ch->mute ? ch->unsetMute(false) : ch->setMute(false);

	if (!gui) {
		Fl::lock();
		ch->guiChannel->mute->value(ch->mute);
		Fl::unlock();
	}
}


/* -------------------------------------------------------------------------- */


void toggleSolo(Channel* ch, bool gui)
{
	ch->solo ? setSolo(ch, true, gui) : setSolo(ch, true, gui);
}

/* -------------------------------------------------------------------------- */


void setSolo(Channel* ch, bool v, bool gui)
{
	using namespace giada::m;

	ch->solo = v;
	mh::updateSoloCount();

	if (!gui) {
		Fl::lock();
		ch->guiChannel->solo->value(ch->solo);
		Fl::unlock();
	}
}


/* -------------------------------------------------------------------------- */


void kill(ResourceChannel* ch)
{
	ch->kill(0); // on frame 0: it's a user-generated event
}


/* -------------------------------------------------------------------------- */


void setBoost(SampleChannel* ch, float val)
{
	ch->setBoost(val);
	gdSampleEditor *gdEditor = static_cast<gdSampleEditor*>(gu_getSubwindow(G_MainWin, WID_SAMPLE_EDITOR));
	if (gdEditor) {
		Fl::lock();
		gdEditor->boostTool->refresh();
		Fl::unlock();
	}
}


/* -------------------------------------------------------------------------- */


void setName(Channel* ch, const string& name)
{
	ch->name = name;
	if (ch->guiChannel != nullptr) ch->guiChannel->update();

	if (ch->type == G_CHANNEL_INPUT) {
		gdInputList* gdInput = static_cast<gdInputList*>(gu_getSubwindow(G_MainWin, WID_INPUT_LIST));
		if (gdInput) {
			Fl::lock();
			gdInput->refreshList();
			Fl::unlock();
		}
	}
	else if (ch->type == G_CHANNEL_COLUMN) {
		gdColumnList* gdColumn = static_cast<gdColumnList*>(gu_getSubwindow(G_MainWin, WID_COLUMN_LIST));
		if (gdColumn) {
			Fl::lock();
			gdColumn->refreshList();
			Fl::unlock();
		}
	}
}


/* -------------------------------------------------------------------------- */


void toggleReadingRecs(ResourceChannel* ch, bool gui)
{

	/* When you call startReadingRecs with conf::treatRecsAsLoops, the
	member value ch->readActions actually is not set to true immediately, because
	the channel is in wait mode (REC_WAITING). ch->readActions will become true on
	the next first beat. So a 'stop rec' command should occur also when
	ch->readActions is false but the channel is in wait mode; this check will
	handle the case of when you press 'R', the channel goes into REC_WAITING and
	then you press 'R' again to undo the status. */

	if (ch->getReadActions() || (!ch->getReadActions() && ch->recStatus == REC_WAITING))
		stopReadingRecs(ch, gui);
	else
		startReadingRecs(ch, gui);
}


/* -------------------------------------------------------------------------- */


void startReadingRecs(ResourceChannel* ch, bool gui)
{
	using namespace giada::m;

	if (conf::treatRecsAsLoops)
		ch->recStart();
	else
		ch->setReadActions(true, conf::recsStopOnChanHalt);
	if (!gui) {
		Fl::lock();
		// TODO: fix this mess (should not be commented)
		//static_cast<geResourceChannel*>(ch->guiChannel)->readActions->value(1);
		Fl::unlock();
	}
}


/* -------------------------------------------------------------------------- */


void stopReadingRecs(ResourceChannel* ch, bool gui)
{
	using namespace giada::m;

	/* First of all, if the clock is not running just stop and disable everything.
	Then if "treatRecsAsLoop" wait until the sequencer reaches beat 0, so put the
	channel in REC_ENDING status. */
	int recStatus = ch->recStatus;
	if (!clock::isRunning()) {
		recStatus = REC_STOPPED;
		ch->setReadActions(false, false);
	}
	else
	if (recStatus == REC_WAITING)
		recStatus = REC_STOPPED;
	else
	if (recStatus == REC_ENDING)
		recStatus = REC_READING;
	else
	if (conf::treatRecsAsLoops)
		recStatus = REC_ENDING;
	else
		ch->setReadActions(false, conf::recsStopOnChanHalt);

	if (!gui) {
		Fl::lock();
		// TODO: fix this mess (should not be commented)
		//static_cast<geResourceChannel*>(ch->guiChannel)->readActions->value(0);
		Fl::unlock();
	}
}

}}}; // giada::c::channel::
