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
#include "../../glue/plugin.h"
#include "../../utils/log.h"
#include "../../utils/string.h"
#include "../elems/basics/boxtypes.h"
#include "../elems/basics/idButton.h"
#include "../elems/basics/statusButton.h"
#include "../elems/basics/choice.h"
#include "../elems/mainWindow/mainIO.h"
#include "../elems/mainWindow/keyboard/channel.h"
#include "gd_mainWindow.h"
#include "inputList.h"


extern gdMainWindow* G_MainWin;


using std::string;
using namespace giada::m;
using namespace giada::c;


gdInputList::gdInputList()
	: gdWindow(478, 204)
{
	if (conf::inputListX)
		resize(conf::inputListX, conf::inputListY, w(), h());

	list = new Fl_Scroll(8, 8, 476, 188);
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

	/* add new buttons, as many as the plugin in pluginHost::stack + 1,
	 * the 'add new' button. Warning: if ch == nullptr we are working with
	 * master in/master out stacks. */

	
	//int numInputs = pluginHost::countPlugins(stackType, ch);
	//int numInputs = 0;
	int numInputs = mh::getInputChannelCount();
	int i = 0;

	while (i<numInputs) {
		InputChannel *pInput = mh::getInputChannelByIndex(i);
		gdInput *gdi     = new gdInput(this, pInput, list->x(), list->y()-list->yposition()+(i*24), 800);
		list->add(gdi);
		i++;
	}

	int addInputY = numInputs == 0 ? 90 : list->y()-list->yposition()+(i*24);
	addInput = new geButton(8, addInputY, 452, 20, "-- add new input --");
	addInput->callback(cb_addInput, (void*)this);
	list->add(addInput);

	/* if num(plugins) > 7 make room for the side scrollbar.
	 * Scrollbar.width = 20 + 4(margin) */

	if (i>7)
		size(492, h());
	else
		size(468, h());

	redraw();
}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


gdInput::gdInput(gdInputList* gdi, InputChannel* i, int X, int Y, int W)
	: Fl_Group(X, Y, W, 20), pParent(gdi), pInput (i)
{
	begin();
	button  			= new geIdButton(8, y(), 100, 20);
	inputAudio         = new geChoice(button->x()+button->w()+4, y(), 132, 20);
	inputMidiDevice    = new geChoice(inputAudio->x()+inputAudio->w()+4, y(), 132, 20);
	inputMidiChannel   = new geChoice(inputMidiDevice->x()+inputMidiDevice->w()+4, y(), 40, 20);
	remove  = new geIdButton(inputMidiChannel->x()+inputMidiChannel->w()+4, y(), 20, 20, "", fxRemoveOff_xpm, fxRemoveOn_xpm);
	fx    	= new geStatusButton(remove->x()+remove->w()+4, y(), 20, 20, fxOff_xpm, fxOn_xpm);
	end();

	button->copy_label(i->getLabel().c_str());
	button->type(FL_TOGGLE_BUTTON);
	button->value(1);

	inputAudio->add("- no audio source -");
	inputAudio->value(0);

	inputMidiDevice->add("- no midi source -");
	inputMidiDevice->value(0);

	inputMidiChannel->add("All");
	for (int i = 0; i < 16; i++) inputMidiChannel->add(std::to_string(i+1).c_str());
	inputMidiChannel->value(0);
	

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
}


/* -------------------------------------------------------------------------- */

void gdInput::cb_setLabel		   		(Fl_Widget* v, void* p) { ((gdInput*)p)->cb_setLabel(); }
void gdInput::cb_removeInput    		(Fl_Widget* v, void* p) { ((gdInput*)p)->cb_removeInput(); }
void gdInput::cb_setInputAudio		 	(Fl_Widget* v, void* p) { ((gdInput*)p)->cb_setInputAudio(); }
void gdInput::cb_setInputMidiDevice		(Fl_Widget* v, void* p) { ((gdInput*)p)->cb_setInputMidiDevice(); }
void gdInput::cb_setInputMidiChannel	(Fl_Widget* v, void* p) { ((gdInput*)p)->cb_setInputMidiChannel(); }
void gdInput::cb_setBypass      		(Fl_Widget* v, void* p) { ((gdInput*)p)->cb_setBypass(); }

/* -------------------------------------------------------------------------- */


void gdInput::cb_removeInput()
{
	/* any subwindow linked to the plugin must be destroyed first */

	//pParent->delSubWindow(pPlugin->getId());
	//plugin::freePlugin(pParent->ch, pPlugin->getId(), pParent->stackType);
	//pParent->refreshList();
}


/* -------------------------------------------------------------------------- */


void gdInput::cb_setLabel()
{
	
}

/* -------------------------------------------------------------------------- */


void gdInput::cb_setBypass()
{
	
}


/* -------------------------------------------------------------------------- */

void gdInput::cb_setInputAudio()
{
	
}

/* -------------------------------------------------------------------------- */

void gdInput::cb_setInputMidiDevice()
{
	
}
/* -------------------------------------------------------------------------- */

void gdInput::cb_setInputMidiChannel()
{
	
}
