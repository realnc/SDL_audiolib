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

#include <sndfile.h>
#include <SDL_version.h>
#include <SDL_rwops.h>
#include "aulib_debug.h"


static sf_count_t
sfLenCb(void* rwops)
{
#if SDL_VERSION_ATLEAST(1,3,0)
    Sint64 size = SDL_RWsize((SDL_RWops*)rwops);
    if (size < 0) {
        return 0;
    }
    return size;
#else
    int curPos = SDL_RWtell((SDL_RWops*)rwops);
    SDL_RWseek((SDL_RWops*)rwops, 0, RW_SEEK_END);
    int endPos = SDL_RWtell((SDL_RWops*)rwops);
    SDL_RWseek((SDL_RWops*)rwops, curPos, RW_SEEK_SET);
    if (endPos < 0) {
        return 0;
    }
    return endPos;
#endif
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


namespace Aulib {

/// \private
class AudioDecoderSndfile_priv {
    friend class AudioDecoderSndfile;

    AudioDecoderSndfile_priv();
    ~AudioDecoderSndfile_priv();

    SNDFILE* fSndfile;
    SF_INFO fInfo;
    bool fEOF;
    float fDuration;
};

} // namespace Aulib


Aulib::AudioDecoderSndfile_priv::AudioDecoderSndfile_priv()
    : fSndfile(nullptr),
      fEOF(false),
      fDuration(-1.f)
{
}


Aulib::AudioDecoderSndfile_priv::~AudioDecoderSndfile_priv()
{
}


Aulib::AudioDecoderSndfile::AudioDecoderSndfile()
    : d(new AudioDecoderSndfile_priv)
{ }


Aulib::AudioDecoderSndfile::~AudioDecoderSndfile()
{
    if (d->fSndfile) {
        sf_close(d->fSndfile);
    }
    delete d;
}


bool
Aulib::AudioDecoderSndfile::open(SDL_RWops* rwops)
{
    if (d->fSndfile) {
        return true;
    }
    d->fInfo.format = 0;
    SF_VIRTUAL_IO cbs;
    cbs.get_filelen = sfLenCb;
    cbs.seek = sfSeekCb;
    cbs.read = sfReadCb;
    cbs.write = nullptr;
    cbs.tell = sfTellCb;
    d->fSndfile = sf_open_virtual(&cbs, SFM_READ, &d->fInfo, rwops);
    if (d->fSndfile == nullptr) {
        return false;
    }
    d->fDuration = (float)d->fInfo.frames / d->fInfo.samplerate;
    return true;
}


unsigned
Aulib::AudioDecoderSndfile::getChannels() const
{
    return d->fInfo.channels;
}


unsigned
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
    sf_count_t ret = sf_read_float(d->fSndfile, buf, len);
    if (ret == 0) {
        d->fEOF = true;
    }
    return ret;
}


bool
Aulib::AudioDecoderSndfile::rewind()
{
    return sf_seek(d->fSndfile, 0, SEEK_SET) == 0;
}


float
Aulib::AudioDecoderSndfile::duration() const
{
    return d->fDuration;
}


bool
Aulib::AudioDecoderSndfile::seekToTime(float seconds)
{
    if (sf_seek(d->fSndfile, seconds * d->fInfo.samplerate, SEEK_SET) == -1) {
        return false;
    }
    return true;
}
