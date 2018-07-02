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


#include "../../glue/channel.h"
#include "../../utils/gui.h"
#include "../../core/const.h"
#include "../../core/mixer.h"
#include "../../core/conf.h"
#include "../../core/channel.h"
#include "../../core/inputChannel.h"
#include "../../core/columnChannel.h"
#include "../elems/basics/button.h"
#include "../elems/basics/choice.h"
#include "channelSelectInput.h"


using namespace giada;


gdChannelSelectInput::gdChannelSelectInput(ColumnChannel* ch)
: gdWindow(400, 64, "Select channel input"),
  m_ch    (ch)
{
	using namespace giada::m;

	if (conf::columnInputX)
		resize(conf::columnInputX, conf::columnInputY, w(), h());

	set_modal();

  m_input = new geChoice(G_GUI_OUTER_MARGIN, G_GUI_OUTER_MARGIN, w() - (G_GUI_OUTER_MARGIN * 2), G_GUI_UNIT);
	m_ok = new geButton(w() - 70 - G_GUI_OUTER_MARGIN, m_input->y()+m_input->h() + G_GUI_OUTER_MARGIN, 70, G_GUI_UNIT, "Ok");
	end();

  m_input->add("- no input -");
	int inIndex = 0;
	for (unsigned j = 0; j < mixer::inputChannels.size(); j++) {
		m_input->add(mixer::inputChannels[j]->getName().c_str());
		if (mixer::inputChannels[j] == ch->inputChannel) inIndex = j+1;
	}
	m_input->value(inIndex);
	m_input->callback(cb_setInputChannel, (void*)this);

	m_ok->shortcut(FL_Enter);
	m_ok->callback(cb_update, (void*)this);

	gu_setFavicon(this);
	setId(WID_SAMPLE_NAME);
	show();
}


/* -------------------------------------------------------------------------- */


gdChannelSelectInput::~gdChannelSelectInput()
{
	m::conf::columnInputX = x();
	m::conf::columnInputY = y();
}

/* -------------------------------------------------------------------------- */

void gdChannelSelectInput::cb_setInputChannel(Fl_Widget* v, void* p) { ((gdChannelSelectInput*)p)->cb_setInputChannel(); }
void gdChannelSelectInput::cb_update(Fl_Widget* w, void* p) { ((gdChannelSelectInput*)p)->cb_update(); }

/* -------------------------------------------------------------------------- */

void gdChannelSelectInput::cb_setInputChannel() {
  c::channel::setInput(m_ch, m::mixer::inputChannels[m_input->value()-1]);
}

/* -------------------------------------------------------------------------- */

void gdChannelSelectInput::cb_update()
{
	do_callback();
}
