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
#include "Aulib/AudioDecoderVorbis.h"

#include <vorbis/vorbisfile.h>
#include <SDL_rwops.h>

#include "aulib_debug.h"


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
class AudioDecoderVorbis_priv {
    friend class AudioDecoderVorbis;

    AudioDecoderVorbis_priv();
    ~AudioDecoderVorbis_priv();

    OggVorbis_File* fVFHandle;
    int fCurrentSection;
    vorbis_info* fCurrentInfo;
    bool fEOF;
    float fDuration;
};

} // namespace Aulib


Aulib::AudioDecoderVorbis_priv::AudioDecoderVorbis_priv()
    : fVFHandle(nullptr),
      fCurrentSection(0),
      fCurrentInfo(nullptr),
      fEOF(false),
      fDuration(-1.f)
{ }


Aulib::AudioDecoderVorbis_priv::~AudioDecoderVorbis_priv()
{
    delete fVFHandle;
}


Aulib::AudioDecoderVorbis::AudioDecoderVorbis()
    : d(new AudioDecoderVorbis_priv)
{ }


Aulib::AudioDecoderVorbis::~AudioDecoderVorbis()
{
    if (d->fVFHandle) {
        ov_clear(d->fVFHandle);
    }
    delete d;
}


bool
Aulib::AudioDecoderVorbis::open(SDL_RWops* rwops)
{
    // FIXME: Check if already opened.
    ov_callbacks cbs;
    memset(&cbs, 0, sizeof(cbs));
    cbs.read_func = vorbisReadCb;
    cbs.seek_func = vorbisSeekCb;
    cbs.tell_func = vorbisTellCb;
    cbs.close_func = nullptr;
    if (d->fVFHandle) {
        ov_clear(d->fVFHandle);
        delete d->fVFHandle;
    }
    d->fVFHandle = new OggVorbis_File;
    if (ov_open_callbacks(rwops, d->fVFHandle, nullptr, 0, cbs) != 0) {
        return false;
    }
    d->fCurrentInfo = ov_info(d->fVFHandle, -1);
    double len = ov_time_total(d->fVFHandle, -1);
    d->fDuration = len == OV_EINVAL ? -1 : len;
    return true;
}


unsigned
Aulib::AudioDecoderVorbis::getChannels() const
{
    return d->fCurrentInfo ? d->fCurrentInfo->channels : 0;
}


unsigned
Aulib::AudioDecoderVorbis::getRate() const
{
    return d->fCurrentInfo ? d->fCurrentInfo->rate : 0;
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
        long ret = ov_read_float(d->fVFHandle, &out, (len - decSamples) / channels, &d->fCurrentSection);
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
            d->fCurrentInfo = ov_info(d->fVFHandle, -1);
            callAgain = true;
        }
        // Copy samples to output buffer in interleaved format.
        for (int samp = 0; samp < ret; ++samp) {
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
        ret = ov_raw_seek(d->fVFHandle, 0);
        d->fEOF = false;
    } else {
        ret = ov_raw_seek_lap(d->fVFHandle, 0);
    }
    return ret == 0 ? true : false;
}


float
Aulib::AudioDecoderVorbis::duration() const
{
    return d->fDuration;
}


bool
Aulib::AudioDecoderVorbis::seekToTime(float seconds)
{
    if (ov_time_seek_lap(d->fVFHandle, seconds) == 0) {
        return true;
    }
    return false;
}
