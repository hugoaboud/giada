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
#include "../../core/columnChannel.h"
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
#include "columnList.h"
#include "pluginList.h"

extern gdMainWindow* G_MainWin;


using std::string;
using namespace giada::m;
using namespace giada::c;

#define COLUMNLIST_W 535

gdColumnList::gdColumnList()
	: gdWindow(COLUMNLIST_W, 204)
{
	if (conf::columnListX)
		resize(conf::columnListX, conf::columnListY, w(), h());

	list = new Fl_Scroll(8, 8, COLUMNLIST_W, 188);
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
	label("Column Channels");
	gu_setFavicon(this);
	show();
}


/* -------------------------------------------------------------------------- */


gdColumnList::~gdColumnList()
{
	conf::pluginListX = x();
	conf::pluginListY = y();
}


/* -------------------------------------------------------------------------- */


void gdColumnList::cb_addColumn(Fl_Widget* v, void* p) { ((gdColumnList*)p)->cb_addColumn(); }


/* -------------------------------------------------------------------------- */


void gdColumnList::cb_refreshList(Fl_Widget* v, void* p)
{
	/* note: this callback is fired by gdBrowser. Close its window first,
	 * by calling the parent (pluginList) and telling it to delete its
	 * subwindow (i.e. gdBrowser). */

	gdWindow *child = (gdWindow*) v;
	if (child->getParent() != nullptr)
		(child->getParent())->delSubWindow(child);

	/* finally refresh plugin list: void *p is a pointer to gdColumnList.
	 * This callback works even when you click 'x' to close the window...
	 * well, who cares */

	((gdColumnList*)p)->refreshList();
	((gdColumnList*)p)->redraw();
}


/* -------------------------------------------------------------------------- */


void gdColumnList::cb_addColumn()
{
	/* the usual callback that gdWindow adds to each subwindow in this case
	 * is not enough, because when we close the browser the plugin list
	 * must be redrawn. We have a special callback, cb_refreshList, which
	 * we add to gdColumnChooser. It does exactly what we need. */

	/*gdColumnChooser* pc = new gdColumnChooser(conf::pluginChooserX,
			conf::pluginChooserY, conf::pluginChooserW, conf::pluginChooserH,
			stackType, ch);
	addSubWindow(pc);
	pc->callback(cb_refreshList, (void*)this);	// 'this' refers to gdColumnList*/
	channel::addColumnChannel(G_DEFAULT_COLUMN_WIDTH);
}


/* -------------------------------------------------------------------------- */


void gdColumnList::refreshList()
{
	/* delete the previous list */

	list->clear();
	list->scroll_to(0, 0);

	/* add new buttons, as many as the plugin in pluginHost::stack + 1,
	 * the 'add new' button. Warning: if ch == nullptr we are working with
	 * master in/master out stacks. */


	//int numColumns = pluginHost::countPlugins(stackType, ch);
	//int numColumns = 0;
	int numColumns = mixer::columnChannels.size();
	int i = 0;

	while (i<numColumns) {
		ColumnChannel *pColumn = mixer::columnChannels.at(i);
		gdColumn *gdi     = new gdColumn(this, pColumn, list->x(), list->y()-list->yposition()+(i*24), 836);
		list->add(gdi);
		i++;
	}

	int addColumnY = numColumns == 0 ? 90 : list->y()-list->yposition()+(i*24);
	addColumn = new geButton(8, addColumnY, COLUMNLIST_W-16, 20, "-- add new column --");
	addColumn->callback(cb_addColumn, (void*)this);
	list->add(addColumn);

	/* if num(plugins) > 7 make room for the side scrollbar.
	 * Scrollbar.width = 20 + 4(margin) */

	if (i>7)
		size(COLUMNLIST_W+24, h());
	else
		size(COLUMNLIST_W, h());

	redraw();
}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


gdColumn::gdColumn(gdColumnList* gdi, ColumnChannel* i, int X, int Y, int W)
	: Fl_Group(X, Y, W, 20), pParent(gdi), pColumn (i)
{
	begin();
	button  		 = new geIdButton(x(), y(), 120, 20);
	preMute			 = new geStatusButton(button->x()+button->w()+4, y(), 20, 20, muteOff_xpm, muteOn_xpm);
	fx    			 = new geStatusButton(preMute->x()+preMute->w()+4, y(), 20, 20, fxOff_xpm, fxOn_xpm);
	posMute			 = new geStatusButton(fx->x()+fx->w()+4, y(), 20, 20, muteOff_xpm, muteOn_xpm);
	meter   		 = new geSoundMeter(posMute->x()+posMute->w()+4, y()+4, 140, 12);
	output		     = new geChoice(meter->x()+meter->w()+4, y(), 132, 20);
	vol				 = new geDial(output->x()+output->w()+4, y(), 20, 20);
	inputMonitor	 = new geStatusButton(vol->x()+vol->w()+4, y(), 20, 20, channelStop_xpm, channelPlay_xpm);
	end();

	button->copy_label(i->getName().c_str());
	button->callback(cb_button, (void*)this);
	button->value(0);

	preMute->type(FL_TOGGLE_BUTTON);
	preMute->value(i->pre_mute);
	preMute->callback(cb_preMute, (void*)this);

	fx->callback(cb_openFxWindow, (void*)this);

	posMute->type(FL_TOGGLE_BUTTON);
	posMute->value(i->mute);
	posMute->callback(cb_posMute, (void*)this);

	output->add("- no output -");
	// TODO: output list + mono/stereo
	output->add("1");
	output->value(i->outputIndex+1);
	output->callback(cb_setOutput, (void*)this);

	vol->value(i->getVolume());
	vol->callback(cb_changeVol, (void*)this);

	inputMonitor->type(FL_TOGGLE_BUTTON);
	inputMonitor->value(i->inputMonitor);
	inputMonitor->callback(cb_inputMonitor, (void*)this);
}


/* -------------------------------------------------------------------------- */

void gdColumn::cb_button		    (Fl_Widget* v, void* p) { ((gdColumn*)p)->cb_button(); }
void gdColumn::cb_setOutput      	(Fl_Widget* v, void* p) { ((gdColumn*)p)->cb_setOutput(); }
void gdColumn::cb_preMute			(Fl_Widget* v, void* p) { ((gdColumn*)p)->cb_preMute(); }
#ifdef WITH_VST
void gdColumn::cb_openFxWindow		(Fl_Widget* v, void* p) { ((gdColumn*)p)->cb_openFxWindow(); }
#endif
void gdColumn::cb_posMute			(Fl_Widget* v, void* p) { ((gdColumn*)p)->cb_posMute(); }
void gdColumn::cb_changeVol			(Fl_Widget* v, void* p) { ((gdColumn*)p)->cb_changeVol(); }
void gdColumn::cb_inputMonitor		(Fl_Widget* v, void* p) { ((gdColumn*)p)->cb_inputMonitor(); }

/* -------------------------------------------------------------------------- */

void gdColumn::cb_setOutput()
{
	pColumn->outputIndex = output->value()-1;
}

/* -------------------------------------------------------------------------- */

void gdColumn::cb_preMute() {
	if (preMute->value()) {
		pColumn->setPreMute(false);
		posMute->value(pColumn->mute);
	}
	else pColumn->unsetPreMute(false);
}

/* -------------------------------------------------------------------------- */

#ifdef WITH_VST
void gdColumn::cb_openFxWindow() {
	gu_openSubWindow(G_MainWin, new gdPluginList(pColumn), WID_FX_LIST);
}
#endif

/* -------------------------------------------------------------------------- */

void gdColumn::cb_posMute() {
	if (posMute->value()) {
		pColumn->setMute(false);
		preMute->value(pColumn->pre_mute);
	}
	else pColumn->unsetMute(false);
}

/* -------------------------------------------------------------------------- */

void gdColumn::cb_changeVol() {
	gu_log("dialvol: %f\n", vol->value());
	pColumn->setVolume(vol->value());
}

/* -------------------------------------------------------------------------- */

void gdColumn::cb_inputMonitor() {
	pColumn->inputMonitor = inputMonitor->value() > 0;
}

/* -------------------------------------------------------------------------- */

void gdColumnList::refresh() {
	// TODO: this is just terrible and may cause segfaults, fix it
	// (FL_Group has 2 misterious children that resist clear plus the "new column" button)
	for (int i=3; i<list->children(); i++) {
		((gdColumn*)list->child(i-3))->refresh();
	}
}

void gdColumn::refresh() {
	//meter->mixerPeak  = pColumn->getPeak();
	//meter->redraw();
}

enum class Menu
{
	RENAME_CHANNEL = 0,
	DELETE_CHANNEL
};

void columnButtonCallback(Fl_Widget* w, void* v)
{
	using namespace giada;

	gdColumn* gcol = static_cast<gdColumn*>(w);
	Menu selectedItem = (Menu) (intptr_t) v;

	switch (selectedItem) {
		case Menu::RENAME_CHANNEL:
			gu_openSubWindow(G_MainWin, new gdChannelNameInput(gcol->getChannel()), WID_SAMPLE_NAME);
			break;
		case Menu::DELETE_CHANNEL:
			c::channel::deleteChannel(gcol->getChannel());
			break;
	}
}

void gdColumn::cb_button()
{
	using namespace giada;

	Fl_Menu_Item rclick_menu[] = {
		{"Rename column", 		0, columnButtonCallback, (void*) Menu::RENAME_CHANNEL},
		{"Delete column", 		0, columnButtonCallback, (void*) Menu::DELETE_CHANNEL},
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
