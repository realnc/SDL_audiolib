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
#include "Aulib/AudioDecoderOpus.h"
#include <cstring>
#include <opusfile.h>
#include <SDL_rwops.h>
#include "aulib_debug.h"


extern "C" {

static int
opusReadCb(void* rwops, unsigned char* ptr, int nbytes)
{
    return SDL_RWread(static_cast<SDL_RWops*>(rwops), ptr, 1, nbytes);
}


static int
opusSeekCb(void* rwops, opus_int64 offset, int whence)
{
    if (SDL_RWseek(static_cast<SDL_RWops*>(rwops), offset, whence) < 0) {
        return -1;
    }
    return 0;
}


static opus_int64
opusTellCb(void* rwops)
{
    return SDL_RWtell(static_cast<SDL_RWops*>(rwops));
}

} // extern "C"


namespace Aulib {

/// \private
struct AudioDecoderOpus_priv {
    friend class AudioDecoderOpus;

    AudioDecoderOpus_priv();
    ~AudioDecoderOpus_priv();

    OggOpusFile* fOpusHandle;
    OpusFileCallbacks fCbs;
    bool fEOF;
    float fDuration;
};

} // namespace Aulib


Aulib::AudioDecoderOpus_priv::AudioDecoderOpus_priv()
    : fOpusHandle(nullptr),
      fEOF(false),
      fDuration(-1.f)
{
    std::memset(&fCbs, 0, sizeof(fCbs));
    fCbs.read = opusReadCb;
    fCbs.seek = opusSeekCb;
    fCbs.tell = opusTellCb;
    fCbs.close = nullptr;
}


Aulib::AudioDecoderOpus_priv::~AudioDecoderOpus_priv()
{
}


Aulib::AudioDecoderOpus::AudioDecoderOpus()
    : d(std::make_unique<AudioDecoderOpus_priv>())
{ }


Aulib::AudioDecoderOpus::~AudioDecoderOpus()
{
    if (d->fOpusHandle) {
        op_free(d->fOpusHandle);
    }
}


bool
Aulib::AudioDecoderOpus::open(SDL_RWops* rwops)
{
    if (isOpen()) {
        return true;
    }
    int error;
    if ((d->fOpusHandle = op_open_callbacks(rwops, &d->fCbs, nullptr, 0, &error)) == nullptr) {
        AM_debugPrintLn("ERROR:" << error);
        if (error == OP_ENOTFORMAT) {
            AM_debugPrintLn("OP_ENOTFORMAT");
        }
        return false;
    }
    ogg_int64_t len = op_pcm_total(d->fOpusHandle, -1);
    // Opus is always 48kHz.
    d->fDuration = len == OP_EINVAL ? -1 : (float)len / 48000.f;
    setIsOpen(true);
    return true;
}


unsigned
Aulib::AudioDecoderOpus::getChannels() const
{
    return 2;
}


unsigned
Aulib::AudioDecoderOpus::getRate() const
{
    return 48000;
}


size_t
Aulib::AudioDecoderOpus::doDecoding(float buf[], size_t len, bool& callAgain)
{
    callAgain = false;

    if (d->fEOF) {
        return 0;
    }

    size_t decSamples = 0;

    while (decSamples < len) {
        int ret = op_read_float_stereo(d->fOpusHandle, buf + decSamples, len - decSamples);
        if (ret == 0) {
            d->fEOF = true;
            break;
        }
        if (ret < 0) {
            AM_debugPrint("libopusfile stream error: ");
            switch (ret) {
                case OP_HOLE: AM_debugPrintLn("OP_HOLE"); break;
                case OP_EBADLINK: AM_debugPrintLn("OP_EBADLINK"); break;
                case OP_EINVAL: AM_debugPrintLn("OP_EINVAL"); break;
                default: AM_debugPrintLn("unknown error: " << ret);
            }
            break;
        }
        decSamples += ret * 2;
    }
    return decSamples;
}


bool
Aulib::AudioDecoderOpus::rewind()
{
    int ret = op_raw_seek(d->fOpusHandle, 0);
    d->fEOF = false;
    return ret == 0 ? true : false;
}


float
Aulib::AudioDecoderOpus::duration() const
{
    return d->fDuration;
}


bool
Aulib::AudioDecoderOpus::seekToTime(float seconds)
{
    ogg_int64_t offset = seconds * 48000.f;
    if (op_pcm_seek(d->fOpusHandle, offset) == 0) {
        return true;
    }
    return false;
}
