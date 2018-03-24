// This is copyrighted software. More information is at the end of this file.
#include "Aulib/AudioDecoderXmp.h"

#include "Buffer.h"
#include "aulib.h"
#include <limits>
#include <type_traits>
#include <SDL_rwops.h>
#include <xmp.h>

namespace Aulib {

/// \private
struct AudioDecoderXmp_priv final {
    std::unique_ptr<std::remove_pointer_t<xmp_context>, decltype(&xmp_free_context)>
        fContext{nullptr, xmp_free_context};
    int fRate = 0;
    bool fEof = false;
};

} // namespace Aulib


Aulib::AudioDecoderXmp::AudioDecoderXmp()
    : d(std::make_unique<AudioDecoderXmp_priv>())
{ }


Aulib::AudioDecoderXmp::~AudioDecoderXmp() = default;


bool
Aulib::AudioDecoderXmp::open(SDL_RWops* rwops)
{
    if (isOpen()) {
        return true;
    }

    //FIXME: error reporting
    d->fContext.reset(xmp_create_context());
    if (not d->fContext) {
        return false;
    }

    Sint64 dataSize = SDL_RWsize(rwops);
    if (dataSize <= 0 or dataSize > std::numeric_limits<int>::max()) {
        return false;
    }
    Buffer<Uint8> data(dataSize);
    if (SDL_RWread(rwops, data.get(), data.size(), 1) != 1) {
        return false;
    }
    if (xmp_load_module_from_memory(d->fContext.get(), data.get(), data.size()) != 0) {
        return false;
    }
    // libXMP supports 8-48kHz.
    d->fRate = std::min(std::max(8000, Aulib::spec().freq), 48000);
    if (xmp_start_player(d->fContext.get(), d->fRate, 0) != 0) {
        return false;
    }
    setIsOpen(true);
    return true;
}


int
Aulib::AudioDecoderXmp::getChannels() const
{
    return 2;
}


int
Aulib::AudioDecoderXmp::getRate() const
{
    return d->fRate;
}


bool
Aulib::AudioDecoderXmp::rewind()
{
    xmp_restart_module(d->fContext.get());
    return true;
}


float
Aulib::AudioDecoderXmp::duration() const
{
    return -1.f; // TODO
}


bool
Aulib::AudioDecoderXmp::seekToTime(float seconds)
{
    return xmp_seek_time(d->fContext.get(), seconds * 1000) >= 0;
}


int
Aulib::AudioDecoderXmp::doDecoding(float buf[], int len, bool& callAgain)
{
    callAgain = false;
    if (d->fEof) {
        return 0;
    }
    Buffer<Sint16> tmpBuf(len);
    auto ret = xmp_play_buffer(d->fContext.get(), tmpBuf.get(), len * 2, 1);
    // Convert from 16-bit to float.
    for (int i = 0; i < len; ++i) {
        buf[i] = tmpBuf[i] / 32768.f;
    }
    if (ret == -XMP_END) {
        d->fEof = true;
    }
    if (ret < 0) {
        return 0;
    }
    return len;
}


/*

Copyright (C) 2014, 2015, 2016, 2017, 2018 Nikos Chantziaras.

This file is part of SDL_audiolib.

SDL_audiolib is free software: you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option) any
later version.

SDL_audiolib is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details.

You should have received a copy of the GNU Lesser General Public License
along with SDL_audiolib. If not, see <http://www.gnu.org/licenses/>.

*/
