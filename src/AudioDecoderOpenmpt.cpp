/*
  SDL_audiolib - An audio decoding, resampling and mixing library
  Copyright (C) 2014  Nikos Chantziaras

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/
#include "Aulib/AudioDecoderOpenmpt.h"
#include "Buffer.h"
#include "aulib.h"
#include <libopenmpt/libopenmpt.hpp>
#include <memory>
#include <limits>
#include <SDL_rwops.h>

namespace Aulib {

/// \private
struct AudioDecoderOpenmpt_priv final {
    std::unique_ptr<openmpt::module> fModule = nullptr;
    bool atEOF = false;
    float fDuration = -1.f;
};

} // namespace Aulib


Aulib::AudioDecoderOpenmpt::AudioDecoderOpenmpt()
    : d(std::make_unique<AudioDecoderOpenmpt_priv>())
{ }


Aulib::AudioDecoderOpenmpt::~AudioDecoderOpenmpt()
{ }


bool
Aulib::AudioDecoderOpenmpt::open(SDL_RWops* rwops)
{
    if (isOpen()) {
        return true;
    }
    //FIXME: error reporting
    Sint64 dataSize = SDL_RWsize(rwops);
    if (dataSize <= 0 or dataSize > std::numeric_limits<int>::max()) {
        return false;
    }
    Buffer<Uint8> data(dataSize);
    if (SDL_RWread(rwops, data.get(), data.size(), 1) != 1) {
        return false;
    }

    std::unique_ptr<openmpt::module> module(nullptr);
    try {
        module = std::make_unique<openmpt::module>(data.get(), data.size());
    } catch (const openmpt::exception& e) {
        AM_warnLn("libopenmpt failed to load mod: " << e.what());
        return false;
    }

    d->fDuration = module->get_duration_seconds();
    d->fModule.swap(module);
    setIsOpen(true);
    return true;
}


int
Aulib::AudioDecoderOpenmpt::getChannels() const
{
    return Aulib::spec().channels;
}


int
Aulib::AudioDecoderOpenmpt::getRate() const
{
    return Aulib::spec().freq;
}


bool
Aulib::AudioDecoderOpenmpt::rewind()
{
    d->fModule->set_position_seconds(0.0);
    d->atEOF = false;
    return true;
}


float
Aulib::AudioDecoderOpenmpt::duration() const
{
    return d->fDuration;
}


bool
Aulib::AudioDecoderOpenmpt::seekToTime(float seconds)
{
    d->fModule->set_position_seconds(seconds);
    return true;
}


int
Aulib::AudioDecoderOpenmpt::doDecoding(float buf[], int len, bool& callAgain)
{
    callAgain = false;
    if (d->atEOF) {
        return 0;
    }
    int ret;
    if (Aulib::spec().channels == 2) {
        ret = d->fModule->read_interleaved_stereo(Aulib::spec().freq, len / 2, buf) * 2;
    } else {
        AM_debugAssert(Aulib::spec().channels == 1);
        ret = d->fModule->read(Aulib::spec().freq, len, buf);
    }
    if (ret == 0) {
        d->atEOF = true;
    }
    return ret;
}