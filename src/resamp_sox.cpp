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
#include "resamp_sox.h"

#include <cstring>
#include <soxr.h>
#include "aulib_debug.h"


namespace Aulib {

/// \private
class AudioResamplerSox_priv {
    friend class AudioResamplerSox;

    AudioResamplerSox_priv();
    ~AudioResamplerSox_priv();

    soxr_t fResampler;
};

} // namespace Aulib


Aulib::AudioResamplerSox_priv::AudioResamplerSox_priv()
    : fResampler(nullptr)
{
}


Aulib::AudioResamplerSox_priv::~AudioResamplerSox_priv()
{
    if (fResampler) {
        soxr_delete(fResampler);
    }
}


Aulib::AudioResamplerSox::AudioResamplerSox()
    : d(new AudioResamplerSox_priv)
{
}


Aulib::AudioResamplerSox::~AudioResamplerSox()
{
    delete d;
}


void
Aulib::AudioResamplerSox::doResampling(float dst[], const float src[], int& dstLen, int& srcLen)
{
    if (d->fResampler == nullptr) {
        dstLen = srcLen = 0;
        return;
    }
    size_t dstDone, srcDone, channels = currentChannels();
    soxr_error_t error;
    error = soxr_process(d->fResampler, src, srcLen / channels, &srcDone, dst, dstLen / channels,
                         &dstDone);
    if (error) {
        // FIXME: What do we do?
        AM_warnLn("soxr_process() error: " << error);
        dstLen = srcLen = 0;
        return;
    }
    dstLen = dstDone * channels;
    srcLen = srcDone * channels;
}


int
Aulib::AudioResamplerSox::adjustForOutputSpec(unsigned dstRate, unsigned srcRate, unsigned channels)
{
    if (d->fResampler) {
        soxr_delete(d->fResampler);
    }

    soxr_io_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.itype = spec.otype = SOXR_FLOAT32_I;
    spec.scale = 1.0;

    //soxr_quality_spec_t quality = soxr_quality_spec()

    soxr_error_t error;
    d->fResampler = soxr_create(srcRate, dstRate, channels, &error, &spec, nullptr, nullptr);
    if (error) {
        d->fResampler = nullptr;
        return -1;
    }
    return 0;
}
