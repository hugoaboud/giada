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
class geStatusButton;
class geChoice;
class geSoundMeter;
class geDial;
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
	void refresh();
};


/* -------------------------------------------------------------------------- */


class gdInput : public Fl_Group
{
  private:

    	static void cb_button(Fl_Widget *v, void *p);
    	static void cb_setBypass(Fl_Widget *v, void *p);
  	static void cb_setInputAudio(Fl_Widget *v, void *p);
  	static void cb_setInputMidiDevice(Fl_Widget *v, void *p);
  	static void cb_setInputMidiChannel(Fl_Widget *v, void *p);
  #ifdef WITH_VST
  	static void cb_openFxWindow(Fl_Widget* v, void* p);
  #endif
  	static void cb_preMute(Fl_Widget* v, void* p);
  	static void cb_posMute(Fl_Widget* v, void* p);
  	static void cb_changeVol(Fl_Widget* v, void* p);
  	static void cb_inputMonitor(Fl_Widget* v, void* p);

  	void cb_button();
  	void cb_setBypass();
  	void cb_setInputAudio();
  	void cb_setInputMidiDevice();
  	void cb_setInputMidiChannel();
  #ifdef WITH_VST
  	void cb_openFxWindow();
  #endif
  	void cb_preMute();
  	void cb_posMute();
  	void cb_changeVol();
  	void cb_inputMonitor();

  public:

    InputChannel *ch;

  	geIdButton     *remove;
  	geIdButton	   *button;
  	geChoice	   *inputAudio;
  	geChoice	   *inputMidiDevice;
  	geChoice	   *inputMidiChannel;
  #ifdef WITH_VST
  	geStatusButton *fx;
  #endif
  	geStatusButton *preMute;
  	geStatusButton *posMute;
  	geSoundMeter   *meter;
  	geDial         *vol;
  	geStatusButton *inputMonitor;

  	gdInput(gdInputList *gdi, InputChannel *i, int x, int y, int w);
  	void refresh();
    void update();

};

#endif
