/* -----------------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ge_modeBox
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
#include "../../../../utils/gui.h"
#include "../../../../core/graphics.h"
#include "../../../../core/resourceChannel.h"
#include "../../../../core/const.h"
#include "../../basics/boxtypes.h"
#include "channelRecMode.h"


geChannelRecMode::geChannelRecMode(int x, int y, int w, int h, ResourceChannel *ch,
  const char *L)
  : Fl_Menu_Button(x, y, w, h, L), ch(ch)
{
  box(G_CUSTOM_BORDER_BOX);
  textsize(G_GUI_FONT_SIZE_BASE);
  textcolor(G_COLOR_LIGHT_2);
  color(G_COLOR_GREY_2);

  add("Single . zero",              0, cb_changeMode, (void *)REC_SINGLE_ZERO);
  add("Single . zero . play",       0, cb_changeMode, (void *)REC_SINGLE_ZERO_PLAY);
  add("Single . quantize",          0, cb_changeMode, (void *)REC_SINGLE_Q);
  add("Single . quantize . play",   0, cb_changeMode, (void *)REC_SINGLE_Q_PLAY);
  add("Overdub . zero",             0, cb_changeMode, (void *)REC_OVERDUB_ZERO);
  add("Overdub . zero . play",      0, cb_changeMode, (void *)REC_OVERDUB_ZERO_PLAY);
  add("Overdub . quantize",         0, cb_changeMode, (void *)REC_OVERDUB_Q);
  add("Overdub . quantize . play",  0, cb_changeMode, (void *)REC_OVERDUB_Q_PLAY);

}


/* -------------------------------------------------------------------------- */


void geChannelRecMode::draw() {
  fl_rect(x(), y(), w(), h(), G_COLOR_GREY_4);    // border
  switch (ch->recMode) {
    case REC_SINGLE_ZERO:
      fl_draw_pixmap(oneshotBasic_xpm, x()+1, y()+1);
      break;
    case REC_SINGLE_ZERO_PLAY:
      fl_draw_pixmap(oneshotRetrig_xpm, x()+1, y()+1);
      break;
    case REC_SINGLE_Q:
      fl_draw_pixmap(oneshotPress_xpm, x()+1, y()+1);
      break;
    case REC_SINGLE_Q_PLAY:
      fl_draw_pixmap(oneshotEndless_xpm, x()+1, y()+1);
      break;
    case REC_OVERDUB_ZERO:
      fl_draw_pixmap(loopBasic_xpm, x()+1, y()+1);
      break;
    case REC_OVERDUB_ZERO_PLAY:
      fl_draw_pixmap(loopOnceBar_xpm, x()+1, y()+1);
      break;
    case REC_OVERDUB_Q:
      fl_draw_pixmap(loopOnce_xpm, x()+1, y()+1);
      break;
    case REC_OVERDUB_Q_PLAY:
      fl_draw_pixmap(loopRepeat_xpm, x()+1, y()+1);
      break;
  }
}


/* -------------------------------------------------------------------------- */


void geChannelRecMode::cb_changeMode(Fl_Widget *v, void *p) { ((geChannelRecMode*)v)->__cb_changeMode((intptr_t)p); }


/* -------------------------------------------------------------------------- */


void geChannelRecMode::__cb_changeMode(int mode)
{
  ch->recMode = mode;

  /* what to do when the channel is playing and you change the mode?
   * Nothing, since v0.5.3. Just refresh the action editor window, in
   * case it's open */

  gu_refreshActionEditor();
}
