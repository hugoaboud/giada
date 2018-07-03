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

#ifndef G_METRONOME_H
#define G_METRONOME_H

#include <initializer_list>
#include <algorithm>

namespace giada {
namespace m {

class AudioBuffer;

namespace metronome {

struct MetronomeWave {
  int id;
  int size;
  float* tick;
  float* tock;
  MetronomeWave(int id, int size, float* tick, float* tock): id(id), size(size), tick(tick), tock(tock) {}
  MetronomeWave(int id, int size, std::initializer_list<float> tick, std::initializer_list<float> tock) {
    this->id = id;
    this->size = size;
    this->tick = new float[tick.size()];
    std::copy(tick.begin(), tick.end(), this->tick);
    this->tock = new float[tock.size()];
    std::copy(tock.begin(), tock.end(), this->tock);
  }
};

void render(AudioBuffer& outBuf, unsigned frame);

extern MetronomeWave defaultWave, tigerWave;

extern MetronomeWave wave;
extern bool   on;
extern float  vol;
extern int    output;
extern int    tickTracker, tockTracker;
extern bool   tickPlay, tockPlay;

}}} // giada::m::metronome::;

#endif
