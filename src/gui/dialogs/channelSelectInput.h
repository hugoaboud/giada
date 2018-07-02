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


#ifndef GD_CHANNEL_SELECT_INPUT_H
#define GD_CHANNEL_SELECT_INPUT_H


#include "window.h"


class Channel;
class geChoice;
class geButton;


class gdChannelSelectInput : public gdWindow
{
private:

	static void cb_setInputChannel(Fl_Widget* v, void* p);
	static void cb_update(Fl_Widget* w, void* p);
	void cb_setInputChannel();
	void cb_update();

	ColumnChannel* m_ch;

	geChoice	   *m_input;
	geButton* m_ok;

public:

	gdChannelSelectInput(ColumnChannel* ch);
	~gdChannelSelectInput();
};

#endif
