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
#include "Aulib/AudioDecoderMpg123.h"

#include <unistd.h>
#include <mpg123.h>
#include <SDL_rwops.h>
#include <SDL_audio.h>
#include "aulib_global.h"
#include "aulib_debug.h"


static bool initialized = false;


static int
initLibMpg()
{
    if (mpg123_init() != MPG123_OK) {
        return -1;
    }
    initialized = true;
    return 0;
}


static int
initMpgFormats(mpg123_handle* handle)
{
    const long* list;
    size_t len;
    mpg123_rates(&list, &len);
    mpg123_format_none(handle);
    for (size_t i = 0; i < len; ++i) {
        if (mpg123_format(handle, list[i], MPG123_STEREO | MPG123_MONO, MPG123_ENC_FLOAT_32)
            != MPG123_OK)
        {
            return -1;
        }
    }
    return 0;
}


static ssize_t
mpgReadCallback(void* rwops, void* buf, size_t len)
{
    return SDL_RWread(static_cast<SDL_RWops*>(rwops), buf, 1, len);
}


static off_t
mpgSeekCallback(void* rwops, off_t pos, int whence)
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
    return SDL_RWseek(static_cast<SDL_RWops*>(rwops), pos, whence);
}


namespace Aulib {

/// \private
class AudioDecoderMpg123_priv {
    friend class AudioDecoderMpg123;

    AudioDecoderMpg123_priv();
    ~AudioDecoderMpg123_priv();

    mpg123_handle* fMpgHandle;
    unsigned fChannels;
    unsigned fRate;
    bool fEOF;
    float fDuration;
};

} // namespace Aulib


Aulib::AudioDecoderMpg123_priv::AudioDecoderMpg123_priv()
    : fMpgHandle(nullptr),
      fChannels(0),
      fRate(0),
      fEOF(false),
      fDuration(-1.f)
{
    if (not initialized) {
        initLibMpg();
    }
}


Aulib::AudioDecoderMpg123_priv::~AudioDecoderMpg123_priv()
{
    mpg123_delete(fMpgHandle);
}


Aulib::AudioDecoderMpg123::AudioDecoderMpg123()
    : d(new AudioDecoderMpg123_priv)
{ }


Aulib::AudioDecoderMpg123::~AudioDecoderMpg123()
{
    delete d;
}


bool
Aulib::AudioDecoderMpg123::open(SDL_RWops* rwops)
{
    if (d->fMpgHandle) {
        return true;
    }
    if (not initialized) {
        return false;
    }
    if ((d->fMpgHandle = mpg123_new(nullptr, nullptr)) == nullptr) {
        return false;
    }
    if (initMpgFormats(d->fMpgHandle) < 0) {
        mpg123_delete(d->fMpgHandle);
        d->fMpgHandle = nullptr;
        return false;
    }
    mpg123_replace_reader_handle(d->fMpgHandle, mpgReadCallback, mpgSeekCallback, nullptr);
    mpg123_open_handle(d->fMpgHandle, rwops);
    long rate;
    int channels, encoding;
    if (mpg123_getformat(d->fMpgHandle, &rate, &channels, &encoding) != 0) {
        mpg123_delete(d->fMpgHandle);
        d->fMpgHandle = nullptr;
        return false;
    }
    d->fChannels = channels;
    d->fRate = rate;
    off_t len = mpg123_length(d->fMpgHandle);
    d->fDuration = (len == MPG123_ERR) ? -1 : ((float)len / rate);
    return true;
}


unsigned
Aulib::AudioDecoderMpg123::getChannels() const
{
    return d->fChannels;
}


unsigned
Aulib::AudioDecoderMpg123::getRate() const
{
    return d->fRate;
}


int
Aulib::AudioDecoderMpg123::doDecoding(float buf[], int len, bool& callAgain)
{
    callAgain = false;
    if (d->fEOF) {
        return 0;
    }

    size_t bytesWanted = static_cast<size_t>(len) * sizeof(*buf);
    size_t decBytes = 0;
    size_t totalBytes = 0;

    while (totalBytes < bytesWanted and not callAgain) {
        int ret = mpg123_read(d->fMpgHandle, (unsigned char*)buf, bytesWanted, &decBytes);
        totalBytes += decBytes;
        if (ret == MPG123_NEW_FORMAT) {
            long rate;
            int channels, encoding;
            mpg123_getformat(d->fMpgHandle, &rate, &channels, &encoding);
            d->fChannels = channels;
            d->fRate = rate;
            callAgain = true;
        } else if (ret == MPG123_DONE) {
            d->fEOF = true;
            break;
        }
    }
    return totalBytes / sizeof(*buf);
}


bool
Aulib::AudioDecoderMpg123::rewind()
{
    if (mpg123_seek(d->fMpgHandle, 0, SEEK_SET) < 0) {
        return false;
    }
    d->fEOF = false;
    return true;
}


float
Aulib::AudioDecoderMpg123::duration() const
{
    return d->fDuration;
}


bool
Aulib::AudioDecoderMpg123::seekToTime(float seconds)
{
    off_t targetFrame = mpg123_timeframe(d->fMpgHandle, seconds);
    if (targetFrame >= 0 and mpg123_seek_frame(d->fMpgHandle, targetFrame, SEEK_SET) >= 0) {
        return true;
    }
    return false;
}
