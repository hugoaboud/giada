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

#ifndef GD_COLUMNLIST_H
#define GD_COLUMNLIST_H

#include <FL/Fl.H>
#include <FL/Fl_Scroll.H>
#include "window.h"


class Plugin;
class Channel;
class ColumnChannel;
class geButton;
class gdColumnList;
class geIdButton;
class geStatusButton;
class geChoice;
class geSoundMeter;
class geDial;
#ifdef WITH_VST
class geStatusButton;
#endif

class gdColumnList : public gdWindow
{
private:

	geButton  *addColumn;
	Fl_Scroll *list;

	static void cb_addColumn(Fl_Widget *v, void *p);
	void cb_addColumn();

public:

	gdColumnList();
	~gdColumnList();

	/* special callback, passed to browser. When closed (i.e. plugin
	 * has been selected) the same browser will refresh this window. */

	static void cb_refreshList(Fl_Widget*, void*);

	void refreshList();
	void refresh();
};


/* -------------------------------------------------------------------------- */


class gdColumn : public Fl_Group
{
private:

  gdColumnList  *pParent;
  ColumnChannel *pColumn;

  	static void cb_button(Fl_Widget *v, void *p);
  	static void cb_setOutput(Fl_Widget* v, void* p);
	static void cb_preMute(Fl_Widget* v, void* p);
#ifdef WITH_VST
	static void cb_openFxWindow(Fl_Widget* v, void* p);
#endif
	static void cb_posMute(Fl_Widget* v, void* p);
	static void cb_changeVol(Fl_Widget* v, void* p);
	static void cb_inputMonitor(Fl_Widget* v, void* p);

	void cb_button();
	void cb_setOutput();
	void cb_preMute();
#ifdef WITH_VST
	void cb_openFxWindow();
#endif
	void cb_posMute();
	void cb_changeVol();
	void cb_inputMonitor();

public:

	geIdButton	   *button;
	geStatusButton *preMute;
#ifdef WITH_VST
	geStatusButton *fx;
#endif
	geStatusButton *posMute;
	geChoice	   *output;
	geSoundMeter   *meter;
	geDial         *vol;
	geStatusButton *inputMonitor;

	gdColumn(gdColumnList *gdi, ColumnChannel *i, int x, int y, int w);
	void refresh();

	ColumnChannel* getChannel() { return pColumn; }
};

#endif
