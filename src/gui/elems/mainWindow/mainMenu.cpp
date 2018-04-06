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


#include <FL/Fl_Menu_Button.H>
#include "../../../core/const.h"
#include "../../../core/mixer.h"
#include "../../../core/mixerHandler.h"
#include "../../../core/conf.h"
#include "../../../core/patch.h"
#include "../../../core/channel.h"
#include "../../../core/columnChannel.h"
#include "../../../core/sampleChannel.h"
#include "../../../utils/gui.h"
#include "../../../glue/storage.h"
#include "../../../glue/main.h"
#include "../../elems/basics/boxtypes.h"
#include "../../elems/basics/button.h"
#include "../../dialogs/gd_mainWindow.h"
#include "../../dialogs/gd_about.h"
#include "../../dialogs/gd_config.h"
#include "../../dialogs/gd_warnings.h"
#include "../../dialogs/browser/browserLoad.h"
#include "../../dialogs/browser/browserSave.h"
#include "../../dialogs/midiIO/midiInputMaster.h"
#include "../../dialogs/inputList.h"
#include "../../dialogs/columnList.h"
#include "keyboard/keyboard.h"
#include "mainMenu.h"


extern gdMainWindow* G_MainWin;


using namespace giada::m;


geMainMenu::geMainMenu(int x, int y)
	: Fl_Group(x, y, 300, 20)
{
	begin();

	file   = new geButton(x, y, 70, 21, "file");
	edit   = new geButton(file->x()+file->w()+4,  y, 70, 21, "edit");
	config = new geButton(edit->x()+edit->w()+4, y, 70, 21, "config");
	about	 = new geButton(config->x()+config->w()+4, y, 70, 21, "about");

	end();

	resizable(nullptr);   // don't resize any widget

	about->callback(cb_about, (void*)this);
	file->callback(cb_file, (void*)this);
	edit->callback(cb_edit, (void*)this);
	config->callback(cb_config, (void*)this);
}


/* -------------------------------------------------------------------------- */


void geMainMenu::cb_about (Fl_Widget* v, void* p) { ((geMainMenu*)p)->__cb_about(); }
void geMainMenu::cb_config(Fl_Widget* v, void* p) { ((geMainMenu*)p)->__cb_config(); }
void geMainMenu::cb_file  (Fl_Widget* v, void* p) { ((geMainMenu*)p)->__cb_file(); }
void geMainMenu::cb_edit  (Fl_Widget* v, void* p) { ((geMainMenu*)p)->__cb_edit(); }


/* -------------------------------------------------------------------------- */


void geMainMenu::__cb_about()
{
	gu_openSubWindow(G_MainWin, new gdAbout(), WID_ABOUT);
}


/* -------------------------------------------------------------------------- */


void geMainMenu::__cb_config()
{
	gu_openSubWindow(G_MainWin, new gdConfig(400, 370), WID_CONFIG);
}


/* -------------------------------------------------------------------------- */


void geMainMenu::__cb_file()
{
	/* An Fl_Menu_Button is made of many Fl_Menu_Item */

	Fl_Menu_Item menu[] = {
		{"Open patch or project..."},
		{"Save patch..."},
		{"Save project..."},
		{"Quit Giada"},
		{0}
	};

	Fl_Menu_Button* b = new Fl_Menu_Button(0, 0, 100, 50);
	b->box(G_CUSTOM_BORDER_BOX);
	b->textsize(G_GUI_FONT_SIZE_BASE);
	b->textcolor(G_COLOR_LIGHT_2);
	b->color(G_COLOR_GREY_2);

	const Fl_Menu_Item* m = menu->popup(Fl::event_x(),	Fl::event_y(), 0, 0, b);
	if (!m) return;

	if (strcmp(m->label(), "Open patch or project...") == 0) {
		gdWindow *childWin = new gdBrowserLoad(conf::browserX, conf::browserY,
				conf::browserW, conf::browserH, "Load patch or project",
				conf::patchPath, glue_loadPatch, nullptr);
		gu_openSubWindow(G_MainWin, childWin, WID_FILE_BROWSER);
		return;
	}
	if (strcmp(m->label(), "Save patch...") == 0) {
		if (mh::hasLogicalSamples() || mh::hasEditedSamples())
			if (!gdConfirmWin("Warning", "You should save a project in order to store\nyour takes and/or processed samples."))
				return;
		gdWindow *childWin = new gdBrowserSave(conf::browserX, conf::browserY,
				conf::browserW, conf::browserH, "Save patch",
				conf::patchPath, patch::name, glue_savePatch, nullptr);
		gu_openSubWindow(G_MainWin, childWin, WID_FILE_BROWSER);
		return;
	}
	if (strcmp(m->label(), "Save project...") == 0) {
		gdWindow *childWin = new gdBrowserSave(conf::browserX, conf::browserY,
				conf::browserW, conf::browserH, "Save project",
				conf::patchPath, patch::name, glue_saveProject, nullptr);
		gu_openSubWindow(G_MainWin, childWin, WID_FILE_BROWSER);
		return;
	}
	if (strcmp(m->label(), "Quit Giada") == 0) {
		G_MainWin->do_callback();
		return;
	}
}


/* -------------------------------------------------------------------------- */


void geMainMenu::__cb_edit()
{
	Fl_Menu_Item menu[] = {
		{"Input channels"},
		{"Column channels"},
		{"Clear all samples"},
		{"Clear all actions"},
		{"Remove empty columns"},
		{"Reset to init state"},
		{"Setup global MIDI input..."},
		{0}
	};

	/* clear all actions disabled if no recs, clear all samples disabled
	 * if no samples. */

	menu[3].deactivate();

	for (unsigned i=0; i<mixer::channels.size(); i++)
		if (mixer::channels.at(i)->hasActions) {
			menu[3].activate();
			break;
		}

	for (unsigned i=0; i<mixer::columnChannels.size(); i++) {
		ColumnChannel* cch = mixer::columnChannels.at(i);
		for (unsigned j=0; j<cch->getResourceCount(); j++) {
			SampleChannel* sch = static_cast<SampleChannel*>(cch->getResource(j));
			if (sch != nullptr && sch->wave != nullptr) {
				menu[2].activate();
				break;
			}
		}
	}

	Fl_Menu_Button* b = new Fl_Menu_Button(0, 0, 100, 50);
	b->box(G_CUSTOM_BORDER_BOX);
	b->textsize(G_GUI_FONT_SIZE_BASE);
	b->textcolor(G_COLOR_LIGHT_2);
	b->color(G_COLOR_GREY_2);

	const Fl_Menu_Item* m = menu->popup(Fl::event_x(),	Fl::event_y(), 0, 0, b);
	if (!m) return;

	if (strcmp(m->label(), "Input channels") == 0) {
		gu_openSubWindow(G_MainWin, new gdInputList(), WID_INPUT_LIST);
		return;
	}

	if (strcmp(m->label(), "Column channels") == 0) {
		gu_openSubWindow(G_MainWin, new gdColumnList(), WID_COLUMN_LIST);
		return;
	}

	if (strcmp(m->label(), "Clear all samples") == 0) {
		if (!gdConfirmWin("Warning", "Clear all samples: are you sure?"))
			return;
		G_MainWin->delSubWindow(WID_SAMPLE_EDITOR);
		glue_clearAllSamples();
		return;
	}
	if (strcmp(m->label(), "Clear all actions") == 0) {
		if (!gdConfirmWin("Warning", "Clear all actions: are you sure?"))
			return;
		G_MainWin->delSubWindow(WID_ACTION_EDITOR);
		glue_clearAllActions();
		return;
	}
	if (strcmp(m->label(), "Reset to init state") == 0) {
		if (!gdConfirmWin("Warning", "Reset to init state: are you sure?"))
			return;
		glue_resetToInitState();
		return;
	}
	if (strcmp(m->label(), "Remove empty columns") == 0) {
		G_MainWin->keyboard->organizeColumns();
		return;
	}
	if (strcmp(m->label(), "Setup global MIDI input...") == 0) {
		gu_openSubWindow(G_MainWin, new gdMidiInputMaster(), 0);
		return;
	}
}
