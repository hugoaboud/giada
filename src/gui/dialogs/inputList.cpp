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

#include <FL/fl_draw.H>
#include <FL/Fl_Menu_Button.H>

#include "../../utils/gui.h"
#include "../../utils/fs.h"
#include "../../core/conf.h"
#include "../../core/const.h"
#include "../../core/graphics.h"
#include "../../core/pluginHost.h"
#include "../../core/plugin.h"
#include "../../core/mixer.h"
#include "../../core/mixerHandler.h"
#include "../../core/channel.h"
#include "../../glue/channel.h"
#include "../../glue/plugin.h"
#include "../../utils/log.h"
#include "../../utils/string.h"
#include "../dialogs/channelNameInput.h"
#include "../elems/soundMeter.h"
#include "../elems/basics/dial.h"
#include "../elems/basics/boxtypes.h"
#include "../elems/basics/idButton.h"
#include "../elems/basics/statusButton.h"
#include "../elems/basics/choice.h"
#include "../elems/mainWindow/mainIO.h"
#include "../elems/mainWindow/keyboard/channel.h"
#include "gd_mainWindow.h"
#include "inputList.h"
#include "pluginList.h"

extern gdMainWindow* G_MainWin;

#define INPUTLIST_W 657

using std::string;
using namespace giada::m;
using namespace giada::c;

gdInputList::gdInputList()
	: gdWindow(INPUTLIST_W, 204)
{
	if (conf::inputListX)
		resize(conf::inputListX, conf::inputListY, w(), h());

	list = new Fl_Scroll(8, 8, INPUTLIST_W, 188);
	list->type(Fl_Scroll::VERTICAL);
	list->scrollbar.color(G_COLOR_GREY_2);
	list->scrollbar.selection_color(G_COLOR_GREY_4);
	list->scrollbar.labelcolor(G_COLOR_LIGHT_1);
	list->scrollbar.slider(G_CUSTOM_BORDER_BOX);

	list->begin();
		refreshList();
	list->end();

	end();
	set_non_modal();
	label("Input Channels");
	gu_setFavicon(this);
	show();
}


/* -------------------------------------------------------------------------- */


gdInputList::~gdInputList()
{
	conf::pluginListX = x();
	conf::pluginListY = y();
}


/* -------------------------------------------------------------------------- */


void gdInputList::cb_addInput(Fl_Widget* v, void* p) { ((gdInputList*)p)->cb_addInput(); }


/* -------------------------------------------------------------------------- */


void gdInputList::cb_refreshList(Fl_Widget* v, void* p)
{
	/* note: this callback is fired by gdBrowser. Close its window first,
	 * by calling the parent (pluginList) and telling it to delete its
	 * subwindow (i.e. gdBrowser). */

	gdWindow *child = (gdWindow*) v;
	if (child->getParent() != nullptr)
		(child->getParent())->delSubWindow(child);

	/* finally refresh plugin list: void *p is a pointer to gdInputList.
	 * This callback works even when you click 'x' to close the window...
	 * well, who cares */

	((gdInputList*)p)->refreshList();
	((gdInputList*)p)->redraw();
}


/* -------------------------------------------------------------------------- */


void gdInputList::cb_addInput()
{
	/* the usual callback that gdWindow adds to each subwindow in this case
	 * is not enough, because when we close the browser the plugin list
	 * must be redrawn. We have a special callback, cb_refreshList, which
	 * we add to gdInputChooser. It does exactly what we need. */

	/*gdInputChooser* pc = new gdInputChooser(conf::pluginChooserX,
			conf::pluginChooserY, conf::pluginChooserW, conf::pluginChooserH,
			stackType, ch);
	addSubWindow(pc);
	pc->callback(cb_refreshList, (void*)this);	// 'this' refers to gdInputList*/
	mh::addInputChannel();
	refreshList();
}


/* -------------------------------------------------------------------------- */


void gdInputList::refreshList()
{
	/* delete the previous list */

	list->clear();
	list->scroll_to(0, 0);

	/* add new buttons, as many as the input channels in mixer::inputChannels + 1,
	 * the 'add new' button. Warning: if ch == nullptr we are working with
	 * master in/master out stacks. */

	int numInputs = mixer::inputChannels.size();
	for (int i = 0; i < numInputs; i++) {
		InputChannel *ch = mixer::inputChannels[i];
		gdInput *gdi     = new gdInput(this, ch, list->x(), list->y()-list->yposition()+(i*24), 836);
		list->add(gdi);
	}

	int addInputY = numInputs == 0 ? 90 : list->y()-list->yposition()+(numInputs*24);
	addInput = new geButton(8, addInputY, INPUTLIST_W-16, 20, "-- add new input --");
	addInput->callback(cb_addInput, (void*)this);
	list->add(addInput);

	/* if numInputs > 7 make room for the side scrollbar.
	 * Scrollbar.width = 20 + 4(margin) */

	if (numInputs>7)
		size(INPUTLIST_W+24, h());
	else
		size(INPUTLIST_W, h());

	redraw();
}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


gdInput::gdInput(gdInputList* gdi, InputChannel* i, int X, int Y, int W)
	: Fl_Group(X, Y, W, 20), ch(i) //pParent(gdi), ch (i)
{
	begin();
	button  		 = new geIdButton(x(), y(), 120, 20);
	inputAudio       = new geChoice(button->x()+button->w()+4, y(), 72, 20);
	inputMidiDevice  = new geChoice(inputAudio->x()+inputAudio->w()+4, y(), 132, 20);
	inputMidiChannel = new geChoice(inputMidiDevice->x()+inputMidiDevice->w()+4, y(), 40, 20);
	preMute			 = new geStatusButton(inputMidiChannel->x()+inputMidiChannel->w()+4, y(), 20, 20, muteOff_xpm, muteOn_xpm);
	fx    			 = new geStatusButton(preMute->x()+preMute->w()+4, y(), 20, 20, fxOff_xpm, fxOn_xpm);
	posMute			 = new geStatusButton(fx->x()+fx->w()+4, y(), 20, 20, muteOff_xpm, muteOn_xpm);
	meter   		 = new geSoundMeter(posMute->x()+posMute->w()+4, y()+4, 140, 12);
	vol				 = new geDial(meter->x()+meter->w()+4, y(), 20, 20);
	inputMonitor	 = new geStatusButton(vol->x()+vol->w()+4, y(), 20, 20, channelStop_xpm, channelPlay_xpm);
	end();

	button->copy_label(i->getName().c_str());
	button->callback(cb_button, (void*)this);
	button->value(0);

	inputAudio->callback(cb_setInputAudio, (void*)this);

	inputMidiDevice->add("- no midi source -");
	inputMidiDevice->value(0);

	inputMidiChannel->add("All");
	for (int j = 0; j < 16; j++) inputMidiChannel->add(std::to_string(j+1).c_str());
	inputMidiChannel->value(0);
	inputMidiChannel->callback(cb_setInputMidiChannel, (void*)this);

	preMute->type(FL_TOGGLE_BUTTON);
	preMute->value(i->pre_mute);
	preMute->callback(cb_preMute, (void*)this);

	fx->callback(cb_openFxWindow, (void*)this);

	posMute->type(FL_TOGGLE_BUTTON);
	posMute->value(i->mute);
	posMute->callback(cb_posMute, (void*)this);

	vol->value(i->getVolume());

	inputMonitor->type(FL_TOGGLE_BUTTON);
	inputMonitor->value(i->inputMonitor);
	inputMonitor->callback(cb_inputMonitor, (void*)this);

	/*
	program->callback(cb_setProgram, (void*)this);

	for (int i=0; i<pPlugin->getNumPrograms(); i++)
		program->add(gu_removeFltkChars(pPlugin->getProgramName(i)).c_str());

	if (program->size() == 0) {
		program->add("-- no programs --\0");
		program->deactivate();
	}
	else
		program->value(pPlugin->getCurrentProgram());

	bypass->callback(cb_setBypass, (void*)this);
	bypass->type(FL_TOGGLE_BUTTON);
	bypass->value(pPlugin->isBypassed() ? 0 : 1);

	shiftUp->callback(cb_shiftUp, (void*)this);
	shiftDown->callback(cb_shiftDown, (void*)this);
	remove->callback(cb_removeInput, (void*)this);
	*/

	update();
}


/* -------------------------------------------------------------------------- */

void gdInput::cb_button		    		(Fl_Widget* v, void* p) { ((gdInput*)p)->cb_button(); }
void gdInput::cb_setBypass      		(Fl_Widget* v, void* p) { ((gdInput*)p)->cb_setBypass(); }
void gdInput::cb_setInputAudio		 	(Fl_Widget* v, void* p) { ((gdInput*)p)->cb_setInputAudio(); }
void gdInput::cb_setInputMidiDevice		(Fl_Widget* v, void* p) { ((gdInput*)p)->cb_setInputMidiDevice(); }
void gdInput::cb_setInputMidiChannel	(Fl_Widget* v, void* p) { ((gdInput*)p)->cb_setInputMidiChannel(); }
#ifdef WITH_VST
void gdInput::cb_openFxWindow(Fl_Widget* v, void* p) { ((gdInput*)p)->cb_openFxWindow(); }
#endif
void gdInput::cb_preMute(Fl_Widget* v, void* p) { ((gdInput*)p)->cb_preMute(); }
void gdInput::cb_posMute(Fl_Widget* v, void* p) { ((gdInput*)p)->cb_posMute(); }
void gdInput::cb_changeVol(Fl_Widget* v, void* p) { ((gdInput*)p)->cb_changeVol(); }
void gdInput::cb_inputMonitor(Fl_Widget* v, void* p) { ((gdInput*)p)->cb_inputMonitor(); }



/* -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- */


enum class Menu
{
	RENAME_INPUT = 0,
	DELETE_INPUT,
	MONO_STEREO
};

void inputButtonCallback(Fl_Widget* w, void* v)
{
	gdInput* gin = static_cast<gdInput*>(w);
	Menu selectedItem = (Menu) (intptr_t) v;

	switch (selectedItem) {
		case Menu::RENAME_INPUT:
			gu_openSubWindow(G_MainWin, new gdChannelNameInput(gin->ch), WID_SAMPLE_NAME);
			break;
		case Menu::DELETE_INPUT:
			channel::deleteChannel(gin->ch);
			break;
		case Menu::MONO_STEREO:
			gin->ch->toggleMono();
			gin->update();
			break;
	}
}

void gdInput::cb_button()
{
	using namespace giada;

	Fl_Menu_Item rclick_menu[] = {
		{"Rename input", 		0, inputButtonCallback, (void*) Menu::RENAME_INPUT},
		{"Delete input", 		0, inputButtonCallback, (void*) Menu::DELETE_INPUT},
		{ch->isMono()?"Set Stereo":"Set Mono", 		0, inputButtonCallback, (void*) Menu::MONO_STEREO},
		{0}
	};

	Fl_Menu_Button* b = new Fl_Menu_Button(0, 0, 100, 50);
	b->box(G_CUSTOM_BORDER_BOX);
	b->textsize(G_GUI_FONT_SIZE_BASE);
	b->textcolor(G_COLOR_LIGHT_2);
	b->color(G_COLOR_GREY_2);

	const Fl_Menu_Item* m = rclick_menu->popup(Fl::event_x(), Fl::event_y(), 0, 0, b);
	if (m)
		m->do_callback(this, m->user_data());
	return;
}

/* -------------------------------------------------------------------------- */


void gdInput::cb_setBypass()
{

}

/* -------------------------------------------------------------------------- */

void gdInput::cb_setInputAudio()
{
	if (ch->isMono()) ch->inputIndex = inputAudio->value();
	else ch->inputIndex = inputAudio->value()*2;
}

/* -------------------------------------------------------------------------- */

void gdInput::cb_setInputMidiDevice()
{

}
/* -------------------------------------------------------------------------- */

void gdInput::cb_setInputMidiChannel()
{

}

/* -------------------------------------------------------------------------- */

#ifdef WITH_VST
void gdInput::cb_openFxWindow() {
	gu_openSubWindow(G_MainWin, new gdPluginList(ch), WID_FX_LIST);
}
#endif

/* -------------------------------------------------------------------------- */

void gdInput::cb_preMute() {
	if (preMute->value()) {
		ch->setPreMute(false);
		posMute->value(ch->mute);
	}
	else ch->unsetPreMute(false);
}

/* -------------------------------------------------------------------------- */

void gdInput::cb_posMute() {
	if (posMute->value()) {
		ch->setMute(false);
		preMute->value(ch->pre_mute);
	}
	else ch->unsetMute(false);
}


/* -------------------------------------------------------------------------- */

void gdInput::cb_changeVol() {
	ch->setVolume(vol->value());
}

/* -------------------------------------------------------------------------- */

void gdInput::cb_inputMonitor() {
	ch->inputMonitor = inputMonitor->value() > 0;
}

/* -------------------------------------------------------------------------- */

void gdInputList::refresh() {
	// TODO: this is just terrible and may cause segfaults, fix it
	// (FL_Group has 2 misterious children that resist clear plus the "new input" button)
	for (int i=3; i<list->children(); i++) {
		((gdInput*)list->child(i-3))->refresh();
	}
}

void gdInput::refresh() {
	meter->mixerPeak  = ch->getPeak();
	meter->redraw();
}

/* -------------------------------------------------------------------------- */

void gdInput::update() {
	inputAudio->clear();
	if (ch->isMono()) for (int i = 0; i < conf::channelsIn; i++) inputAudio->add(std::to_string(i+1).c_str());
	else if (conf::channelsIn > 1) for (int i = 0; i < conf::channelsIn-1; i++) inputAudio->add((gu_iToString(i+1) + "-" + gu_iToString(i+2)).c_str());
	else ch->inputIndex = -1;
	inputAudio->value(ch->inputIndex);
}
