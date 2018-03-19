// This is copyrighted software. More information is at the end of this file.
#include "Aulib/AudioDecoderVorbis.h"

#include "aulib_debug.h"
#include <SDL_rwops.h>
#include <vorbis/vorbisfile.h>


extern "C" {

static size_t
vorbisReadCb(void* ptr, size_t size, size_t nmemb, void* rwops)
{
    return SDL_RWread(static_cast<SDL_RWops*>(rwops), ptr, size, nmemb);
}


static int
vorbisSeekCb(void* rwops, ogg_int64_t offset, int whence)
{
    if (SDL_RWseek(static_cast<SDL_RWops*>(rwops), offset, whence) < 0) {
        return -1;
    }
    return 0;
}


static long
vorbisTellCb(void* rwops)
{
    return SDL_RWtell(static_cast<SDL_RWops*>(rwops));
}

} // extern "C"


namespace Aulib {

/// \private
struct AudioDecoderVorbis_priv final {
    std::unique_ptr<OggVorbis_File, decltype(&ov_clear)> fVFHandle{nullptr, ov_clear};
    int fCurrentSection = 0;
    vorbis_info* fCurrentInfo = nullptr;
    bool fEOF = false;
    float fDuration = -1.f;
};

} // namespace Aulib


Aulib::AudioDecoderVorbis::AudioDecoderVorbis()
    : d(std::make_unique<AudioDecoderVorbis_priv>())
{ }


Aulib::AudioDecoderVorbis::~AudioDecoderVorbis() = default;


bool
Aulib::AudioDecoderVorbis::open(SDL_RWops* rwops)
{
    if (isOpen()) {
        return true;
    }
    ov_callbacks cbs{};
    cbs.read_func = vorbisReadCb;
    cbs.seek_func = vorbisSeekCb;
    cbs.tell_func = vorbisTellCb;
    cbs.close_func = nullptr;
    auto newHandle = decltype(d->fVFHandle){new OggVorbis_File, ov_clear};
    if (ov_open_callbacks(rwops, newHandle.get(), nullptr, 0, cbs) != 0) {
        return false;
    }
    d->fCurrentInfo = ov_info(newHandle.get(), -1);
    auto len = ov_time_total(newHandle.get(), -1);
    d->fDuration = len == OV_EINVAL ? -1 : len;
    d->fVFHandle.swap(newHandle);
    setIsOpen(true);
    return true;
}


int
Aulib::AudioDecoderVorbis::getChannels() const
{
    return d->fCurrentInfo != nullptr ? d->fCurrentInfo->channels : 0;
}


int
Aulib::AudioDecoderVorbis::getRate() const
{
    return d->fCurrentInfo != nullptr ? d->fCurrentInfo->rate : 0;
}


int
Aulib::AudioDecoderVorbis::doDecoding(float buf[], int len, bool& callAgain)
{
    callAgain = false;

    if (d->fEOF) {
        return 0;
    }

    float** out;
    int decSamples = 0;

    while (decSamples < len and not callAgain) {
        int lastSection = d->fCurrentSection;
        //TODO: We only support up to 2 channels for now.
        int channels = std::min(d->fCurrentInfo->channels, 2);
        long ret = ov_read_float(d->fVFHandle.get(), &out, (len - decSamples) / channels,
                                 &d->fCurrentSection);
        if (ret == 0) {
            d->fEOF = true;
            break;
        }
        if (ret < 0) {
            AM_debugPrint("libvorbis stream error: ");
            switch (ret) {
                case OV_HOLE: AM_debugPrintLn("OV_HOLE"); break;
                case OV_EBADLINK: AM_debugPrintLn("OV_EBADLINK"); break;
                case OV_EINVAL: AM_debugPrintLn("OV_EINVAL"); break;
                default: AM_debugPrintLn("unknown error: " << ret);
            }
            break;
        }
        if (d->fCurrentSection != lastSection) {
            d->fCurrentInfo = ov_info(d->fVFHandle.get(), -1);
            callAgain = true;
        }
        // Copy samples to output buffer in interleaved format.
        for (long samp = 0; samp < ret; ++samp) {
            for (int chan = 0; chan < channels; ++chan) {
                *buf++ = out[chan][samp];
            }
        }
        decSamples += ret * channels;
    }
    return decSamples;
}


bool
Aulib::AudioDecoderVorbis::rewind()
{
    int ret;
    if (d->fEOF) {
        ret = ov_raw_seek(d->fVFHandle.get(), 0);
        d->fEOF = false;
    } else {
        ret = ov_raw_seek_lap(d->fVFHandle.get(), 0);
    }
    return ret == 0;
}


float
Aulib::AudioDecoderVorbis::duration() const
{
    return d->fDuration;
}


bool
Aulib::AudioDecoderVorbis::seekToTime(float seconds)
{
    return ov_time_seek_lap(d->fVFHandle.get(), seconds) == 0;
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
