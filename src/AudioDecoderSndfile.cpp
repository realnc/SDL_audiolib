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
#include <Aulib/AudioDecoderSndfile.h>

#include "aulib_debug.h"
#include <SDL_rwops.h>
#include <SDL_version.h>
#include <sndfile.h>


extern "C" {

static sf_count_t
sfLenCb(void* rwops)
{
    Sint64 size = SDL_RWsize((SDL_RWops*)rwops);
    if (size < 0) {
        return 0;
    }
    return size;
}


static sf_count_t
sfSeekCb(sf_count_t offset, int whence, void* rwops)
{
    switch (whence) {
        case SEEK_SET:
            whence = RW_SEEK_SET;
            break;
        case SEEK_CUR:
            whence = RW_SEEK_CUR;
            break;
        default:
            whence = RW_SEEK_END;
    }
    int pos = SDL_RWseek((SDL_RWops*)rwops, offset, whence);
    return pos;
}


static sf_count_t
sfReadCb(void* dst, sf_count_t count, void* rwops)
{
    int ret = SDL_RWread((SDL_RWops*)rwops, dst, 1, count);
    return ret;
}


static sf_count_t
sfTellCb(void* rwops)
{
    return SDL_RWtell((SDL_RWops*)rwops);
}

} // extern "C"


namespace Aulib {

/// \private
struct AudioDecoderSndfile_priv final {
    std::unique_ptr<SNDFILE, decltype(&sf_close)> fSndfile{nullptr, &sf_close};
    SF_INFO fInfo{};
    bool fEOF = false;
    float fDuration = -1.f;
};

} // namespace Aulib


Aulib::AudioDecoderSndfile::AudioDecoderSndfile()
    : d(std::make_unique<AudioDecoderSndfile_priv>())
{ }


Aulib::AudioDecoderSndfile::~AudioDecoderSndfile() = default;


bool
Aulib::AudioDecoderSndfile::open(SDL_RWops* rwops)
{
    if (isOpen()) {
        return true;
    }
    d->fInfo.format = 0;
    SF_VIRTUAL_IO cbs;
    cbs.get_filelen = sfLenCb;
    cbs.seek = sfSeekCb;
    cbs.read = sfReadCb;
    cbs.write = nullptr;
    cbs.tell = sfTellCb;
    d->fSndfile.reset(sf_open_virtual(&cbs, SFM_READ, &d->fInfo, rwops));
    if (not d->fSndfile) {
        return false;
    }
    d->fDuration = (float)d->fInfo.frames / d->fInfo.samplerate;
    setIsOpen(true);
    return true;
}


int
Aulib::AudioDecoderSndfile::getChannels() const
{
    return d->fInfo.channels;
}


int
Aulib::AudioDecoderSndfile::getRate() const
{
    return d->fInfo.samplerate;
}


int
Aulib::AudioDecoderSndfile::doDecoding(float buf[], int len, bool& callAgain)
{
    callAgain = false;
    if (d->fEOF) {
        return 0;
    }
    sf_count_t ret = sf_read_float(d->fSndfile.get(), buf, len);
    if (ret == 0) {
        d->fEOF = true;
    }
    return ret;
}


bool
Aulib::AudioDecoderSndfile::rewind()
{
    return sf_seek(d->fSndfile.get(), 0, SEEK_SET) == 0;
}


float
Aulib::AudioDecoderSndfile::duration() const
{
    return d->fDuration;
}


bool
Aulib::AudioDecoderSndfile::seekToTime(float seconds)
{
    return sf_seek(d->fSndfile.get(), seconds * d->fInfo.samplerate, SEEK_SET) != -1;
}
