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


#include <cassert>
#include <FL/fl_draw.H>
#include <FL/Fl_Menu_Button.H>
#include "../../../../core/sampleChannel.h"
#include "../../../../core/columnChannel.h"
#include "../../../../core/midiChannel.h"
#include "../../../../glue/channel.h"
#include "../../../../utils/log.h"
#include "../../../../utils/fs.h"
#include "../../../../utils/string.h"
#include "../../../dialogs/gd_warnings.h"
#include "../../../elems/basics/boxtypes.h"
#include "../../../elems/basics/resizerBar.h"
#include "keyboard.h"
#include "sampleChannel.h"
#include "midiChannel.h"
#include "column.h"


using std::vector;
using std::string;
using namespace giada;

namespace {

enum class Menu
{
	ADD_SAMPLE_CHANNEL = 0,
	ADD_MIDI_CHANNEL,
	RENAME_CHANNEL,
	DELETE_CHANNEL
};

void menuCallback(Fl_Widget* w, void* v)
{
	using namespace giada;

	geColumn* gcol = static_cast<geColumn*>(w);
	Menu selectedItem = (Menu) (intptr_t) v;

	switch (selectedItem) {
		case Menu::ADD_SAMPLE_CHANNEL: {
			c::channel::addResourceChannel(gcol->channel, CHANNEL_SAMPLE, G_GUI_CHANNEL_H_1);
			break;
		}
		case Menu::ADD_MIDI_CHANNEL: {
			c::channel::addResourceChannel(gcol->channel, CHANNEL_MIDI, G_GUI_CHANNEL_H_1);
			break;
		}
		case Menu::RENAME_CHANNEL: {
			//gu_openSubWindow(G_MainWin, new gdChannelNameInput(gch->ch), WID_SAMPLE_NAME);
			break;
		}
		case Menu::DELETE_CHANNEL: {
			c::channel::deleteColumnChannel(gcol->channel);
			break;
		}
	}
}

}

geColumn::geColumn(int X, int Y, int W, int H, int index, geKeyboard* parent, ColumnChannel *channel)
	: geChannel(X, Y, W, H, channel),
		m_parent(parent),
		m_index (index)
{
	/* geColumn does a bit of a mess: we pass a pointer to its m_parent (geKeyboard) and
	the geColumn itself deals with the creation of another widget, outside geColumn
	and inside geKeyboard, which handles the vertical resize bar (geResizerBar).
	The resizer cannot stay inside geColumn: it needs a broader view on the other
	side widgets. The view can be obtained from geKeyboard only (the upper level).
	Unfortunately, parent() can be nullptr: at this point (i.e the constructor)
	geColumn is still detached from any parent. We use a custom geKeyboard *parent
	instead. */

	begin();
		m_titleBtn = new geButton(x(), y(), w(), G_GUI_UNIT, channel->getName().c_str());
	end();

	m_resizer = new geResizerBar(x()+w(), y(), G_GUI_OUTER_MARGIN * 2, h(),
		G_MIN_COLUMN_WIDTH, geResizerBar::HORIZONTAL);
	m_parent->add(m_resizer);

	m_titleBtn->callback(cb_openMenu, (void*)this);
}


/* -------------------------------------------------------------------------- */


geColumn::~geColumn()
{
	/* FIXME - this could actually cause a memory leak. m_resizer is
	just removed, not deleted. But we cannot delete it right now. */

	m_parent->remove(m_resizer);
}


/* -------------------------------------------------------------------------- */



int geColumn::handle(int e)
{
	switch (e) {
		case FL_RELEASE: {
			if (Fl::event_button() == FL_RIGHT_MOUSE) {
				cb_openMenu();
				return 1;
			}
		}
		case FL_DND_ENTER:           	// return(1) for these events to 'accept' dnd
		case FL_DND_DRAG:
		case FL_DND_RELEASE: {
			return 1;
		}
		case FL_PASTE: {              // handle actual drop (paste) operation
			vector<string> paths;
			gu_split(Fl::event_text(), "\n", &paths);
			bool fails = false;
			int result = 0;
			for (string& path : paths) {
				gu_log("[geColumn::handle] loading %s...\n", path.c_str());
				SampleChannel* c = static_cast<SampleChannel*>(c::channel::addResourceChannel(
					channel, G_CHANNEL_SAMPLE, G_GUI_CHANNEL_H_1));
				result = c::channel::loadChannel(c, gu_stripFileUrl(path));
				if (result != G_RES_OK) {
					deleteChannel(c->guiChannel);
					fails = true;
				}
			}
			if (fails) {
				if (paths.size() > 1)
					gdAlert("Some files were not loaded successfully.");
				else
					m_parent->printChannelMessage(result);
			}
			return 1;
		}
	}

	/* we return fl_Group::handle only if none of the cases above are fired. That
	is because we don't want to propagate a dnd drop to all the sub widgets. */

	return Fl_Group::handle(e);
}


/* -------------------------------------------------------------------------- */


void geColumn::resize(int X, int Y, int W, int H)
{
	/* Resize all children, including "add channel" button. */

	for (int i=0; i<children(); i++) {
		Fl_Widget* wgCurr = child(i);
		Fl_Widget* wgPrev = i == 0 ? nullptr : child(i - 1);
		wgCurr->resize(X, (wgPrev == nullptr ? Y : wgPrev->y() + wgPrev->h() + G_GUI_INNER_MARGIN),
			W, wgCurr->h());
	}

	/* Resize group itself. Must use internal functions, resize() would trigger
	infinite recursion. */

	x(X); y(Y); w(W); h(H);

	/* Resize resizerBar. */

	m_resizer->size(G_GUI_OUTER_MARGIN * 2, H);
}


/* -------------------------------------------------------------------------- */


void geColumn::refresh()
{
	for (int i=1; i<children(); i++)
		static_cast<geChannel*>(child(i))->refresh();
}


/* -------------------------------------------------------------------------- */


void geColumn::draw()
{
	fl_color(G_COLOR_GREY_1_5);
	fl_rectf(x(), y(), w(), h());

	/* call draw and then redraw in order to avoid channel corruption when
	scrolling horizontally */

	for (int i=0; i<children(); i++) {
		child(i)->draw();
		child(i)->redraw();
	}
}


/* -------------------------------------------------------------------------- */


void geColumn::cb_openMenu(Fl_Widget* v, void* p) { ((geColumn*)p)->cb_openMenu(); }


/* -------------------------------------------------------------------------- */


void geColumn::repositionChannels()
{
	int totalH = 0;
	for (int i=0; i<children(); i++)
		totalH += child(i)->h() + G_GUI_INNER_MARGIN;
	resize(x(), y(), w(), totalH + 66); // evil space for drag n drop
}


/* -------------------------------------------------------------------------- */


geChannel* geColumn::addChannel(ResourceChannel* ch, int size)
{
	geChannel* gch = nullptr;

	/* All geChannels are added with y=0. That's not a problem, they will be
	repositioned later on during geColumn::resize(). */

	if (ch->type == G_CHANNEL_SAMPLE)
		gch = new geSampleChannel(x(), 0, w(), size, static_cast<SampleChannel*>(ch));
	else
		gch = new geMidiChannel(x(), 0, w(), size, static_cast<MidiChannel*>(ch));

	channel->addResource(ch);
	add(gch);

	repositionChannels();
	gch->redraw();    // fix corruption
	m_parent->redraw(); // redraw Keyboard
	return gch;
}


/* -------------------------------------------------------------------------- */


void geColumn::deleteChannel(geChannel* gch)
{
	gch->hide();
	remove(gch);
	delete gch;

	/** TODO
	 * reposition is useless when called by geColumn::clear(). Add a new
	 * parameter to skip the operation */

	repositionChannels();
}


/* -------------------------------------------------------------------------- */


void geColumn::cb_openMenu()
{
	using namespace giada;

	Fl_Menu_Item rclick_menu[] = {
		{"Add Sample Channel",	0, menuCallback, (void*) Menu::ADD_SAMPLE_CHANNEL},
		{"Add MIDI Channel",	0, menuCallback, (void*) Menu::ADD_MIDI_CHANNEL},
		{"Rename column", 		0, menuCallback, (void*) Menu::RENAME_CHANNEL},
		{"Delete column", 		0, menuCallback, (void*) Menu::DELETE_CHANNEL},
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


void geColumn::clear(bool full)
{
	if (full)
		Fl_Group::clear();
	else {
		while (children() >= 2) {  // skip "add new channel" btn
			int i = children()-1;
			deleteChannel(static_cast<geChannel*>(child(i)));
		}
	}
}


/* -------------------------------------------------------------------------- */


Channel* geColumn::getChannel(int i)
{
	return static_cast<geChannel*>(child(i + 1))->ch;  // Skip "add channel"
}


/* -------------------------------------------------------------------------- */

void geColumn::update() {}
void geColumn::reset() {}

/* -------------------------------------------------------------------------- */

int geColumn::getIndex()       { return m_index; }
void geColumn::setIndex(int i) { m_index = i; }
bool geColumn::isEmpty()       { return children() == 1; }
int geColumn::countChannels()  { return children() - 1; }
