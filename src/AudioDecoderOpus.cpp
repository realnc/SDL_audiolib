// This is copyrighted software. More information is at the end of this file.
#include "Aulib/AudioDecoderOpus.h"

#include "aulib_debug.h"
#include <cstring>
#include <SDL_rwops.h>
#include <opusfile.h>


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
struct AudioDecoderOpus_priv final {
    std::unique_ptr<OggOpusFile, decltype(&op_free)> fOpusHandle{nullptr, &op_free};
    OpusFileCallbacks fCbs{opusReadCb, opusSeekCb, opusTellCb, nullptr};
    bool fEOF = false;
    float fDuration = -1.f;
};

} // namespace Aulib


Aulib::AudioDecoderOpus::AudioDecoderOpus()
    : d(std::make_unique<AudioDecoderOpus_priv>())
{ }


Aulib::AudioDecoderOpus::~AudioDecoderOpus() = default;


bool
Aulib::AudioDecoderOpus::open(SDL_RWops* rwops)
{
    if (isOpen()) {
        return true;
    }
    int error;
    d->fOpusHandle.reset(op_open_callbacks(rwops, &d->fCbs, nullptr, 0, &error));
    if (not d->fOpusHandle) {
        AM_debugPrintLn("ERROR:" << error);
        if (error == OP_ENOTFORMAT) {
            AM_debugPrintLn("OP_ENOTFORMAT");
        }
        return false;
    }
    ogg_int64_t len = op_pcm_total(d->fOpusHandle.get(), -1);
    // Opus is always 48kHz.
    d->fDuration = len == OP_EINVAL ? -1 : len / 48000.f;
    setIsOpen(true);
    return true;
}


int
Aulib::AudioDecoderOpus::getChannels() const
{
    return 2;
}


int
Aulib::AudioDecoderOpus::getRate() const
{
    return 48000;
}


int
Aulib::AudioDecoderOpus::doDecoding(float buf[], int len, bool& callAgain)
{
    callAgain = false;

    if (d->fEOF) {
        return 0;
    }

    int decSamples = 0;

    while (decSamples < len) {
        int ret = op_read_float_stereo(d->fOpusHandle.get(), buf + decSamples, len - decSamples);
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
    int ret = op_raw_seek(d->fOpusHandle.get(), 0);
    d->fEOF = false;
    return ret == 0;
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
    return op_pcm_seek(d->fOpusHandle.get(), offset) == 0;
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
