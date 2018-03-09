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

#ifndef GD_INPUTLIST_H
#define GD_INPUTLIST_H

#include <FL/Fl.H>
#include <FL/Fl_Scroll.H>
#include "window.h"
#include "../../core/inputChannel.h"


class Plugin;
class Channel;
class geButton;
class gdInputList;
class geIdButton;
class geChoice;
#ifdef WITH_VST
class geStatusButton;
#endif

class gdInputList : public gdWindow
{
private:

  geButton  *addInput;
	Fl_Scroll *list;

	static void cb_addInput(Fl_Widget *v, void *p);
	void cb_addInput();

public:

	gdInputList();
	~gdInputList();

	/* special callback, passed to browser. When closed (i.e. plugin
	 * has been selected) the same browser will refresh this window. */

	static void cb_refreshList(Fl_Widget*, void*);

	void refreshList();
};


/* -------------------------------------------------------------------------- */


class gdInput : public Fl_Group
{
private:

  gdInputList  *pParent;
  InputChannel *pInput;

  	static void cb_setLabel(Fl_Widget *v, void *p);
	static void cb_removeInput(Fl_Widget *v, void *p);
	static void cb_setBypass(Fl_Widget *v, void *p);
	static void cb_setInputAudio(Fl_Widget *v, void *p);
	static void cb_setInputMidiDevice(Fl_Widget *v, void *p);
	static void cb_setInputMidiChannel(Fl_Widget *v, void *p);

#ifdef WITH_VST
	static void cb_openFxWindow(Fl_Widget* v, void* p);
#endif

	void cb_setLabel();
	void cb_removeInput();
	void cb_setInputAudio();
	void cb_setInputMidiDevice();
	void cb_setInputMidiChannel();
	void cb_setBypass();

#ifdef WITH_VST
	void cb_openFxWindow();
#endif

public:

	geIdButton	   *button;
	geChoice	   *inputAudio;
	geChoice	   *inputMidiDevice;
	geChoice	   *inputMidiChannel;
	geIdButton     *remove;

#ifdef WITH_VST
	geStatusButton *fx;
#endif

	gdInput(gdInputList *gdi, InputChannel *i, int x, int y, int w);
};

#endif