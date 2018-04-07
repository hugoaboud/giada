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


#include "../../../core/const.h"
#include "../../../core/graphics.h"
#include "../../../core/mixer.h"
#include "../../../core/pluginHost.h"
#include "../../../glue/main.h"
#include "../../../utils/gui.h"
#include "../../elems/soundMeter.h"
#include "../../elems/basics/statusButton.h"
#include "../../elems/basics/dial.h"
#include "../../dialogs/gd_mainWindow.h"
#include "../../dialogs/pluginList.h"
#include "mainIO.h"


extern gdMainWindow *G_MainWin;


using namespace giada::m;


geMainIO::geMainIO(int x, int y)
	: Fl_Group(x, y, 396, 20)
{
	begin();

#if defined(WITH_VST)
	inVol		    = new geDial      (x, y, 20, 20);
	inMeter     = new geSoundMeter(inVol->x()+inVol->w()+4, y+4, 140, 12);
	outMeter    = new geSoundMeter(inMeter->x()+inMeter->w()+4, y+4, 140, 12);
	outVol		  = new geDial      (outMeter->x()+outMeter->w()+4, y, 20, 20);
	masterFxOut = new geStatusButton  (outVol->x()+outVol->w()+4, y, 20, 20, fxOff_xpm, fxOn_xpm);
#else
	inVol		    = new geDial      (x+62, y, 20, 20);
	inMeter     = new geSoundMeter(inVol->x()+inVol->w()+4, y+5, 140, 12);
	outMeter    = new geSoundMeter(inMeter->x()+inMeter->w()+4, y+5, 140, 12);
	outVol		  = new geDial      (outMeter->x()+outMeter->w()+4, y, 20, 20);
#endif

	end();

	resizable(nullptr);   // don't resize any widget

	outVol->callback(cb_outVol, (void*)this);
	outVol->value(mixer::outVol);
	inVol->callback(cb_inVol, (void*)this);
	inVol->value(mixer::inVol);

#ifdef WITH_VST
	masterFxOut->callback(cb_masterFxOut, (void*)this);
#endif
}


/* -------------------------------------------------------------------------- */


void geMainIO::cb_outVol     (Fl_Widget *v, void *p)  	{ ((geMainIO*)p)->__cb_outVol(); }
void geMainIO::cb_inVol      (Fl_Widget *v, void *p)  	{ ((geMainIO*)p)->__cb_inVol(); }
#ifdef WITH_VST
void geMainIO::cb_masterFxOut(Fl_Widget *v, void *p)    { ((geMainIO*)p)->__cb_masterFxOut(); }
#endif


/* -------------------------------------------------------------------------- */


void geMainIO::__cb_outVol()
{
	glue_setOutVol(outVol->value());
}


/* -------------------------------------------------------------------------- */


void geMainIO::__cb_inVol()
{
	glue_setInVol(inVol->value());
}


/* -------------------------------------------------------------------------- */


#ifdef WITH_VST
void geMainIO::__cb_masterFxOut()
{
	//gu_openSubWindow(G_MainWin, new gdPluginList(pluginHost::MASTER_OUT), WID_FX_LIST);
}
#endif


/* -------------------------------------------------------------------------- */


void geMainIO::setOutVol(float v)
{
  outVol->value(v);
}


void geMainIO::setInVol(float v)
{
  inVol->value(v);
}


/* -------------------------------------------------------------------------- */


#ifdef WITH_VST

void geMainIO::setMasterFxOutFull(bool v)
{
  masterFxOut->status = v;
  masterFxOut->redraw();
}


#endif


/* -------------------------------------------------------------------------- */


void geMainIO::refresh()
{
	//outMeter->mixerPeak = mixer::peakOut;
	//inMeter->mixerPeak  = mixer::peakIn;
	//outMeter->redraw();
	//inMeter->redraw();
}
