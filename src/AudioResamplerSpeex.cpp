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
#include "Aulib/AudioResamplerSpeex.h"

#include "Aulib/AudioDecoder.h"
#include "aulib_global.h"
#include "speex_resampler.h"


namespace Aulib {

/// \private
struct AudioResamplerSpeex_priv final {
    std::unique_ptr<SpeexResamplerState, decltype(&speex_resampler_destroy)>
        fResampler{nullptr, &speex_resampler_destroy};
};

} // namespace Aulib


Aulib::AudioResamplerSpeex::AudioResamplerSpeex()
    : d(std::make_unique<AudioResamplerSpeex_priv>())
{ }


Aulib::AudioResamplerSpeex::~AudioResamplerSpeex() = default;


void
Aulib::AudioResamplerSpeex::doResampling(float dst[], const float src[], int& dstLen,
                                         int& srcLen)
{
    if (not d->fResampler) {
        dstLen = srcLen = 0;
        return;
    }

    int channels = currentChannels();
    spx_uint32_t spxInLen = srcLen / channels;
    spx_uint32_t spxOutLen = dstLen / channels;
    if (spxInLen == 0 or spxOutLen == 0) {
        dstLen = srcLen = 0;
        return;
    }
    speex_resampler_process_interleaved_float(d->fResampler.get(), src, &spxInLen, dst, &spxOutLen);
    dstLen = spxOutLen * channels;
    srcLen = spxInLen * channels;
}


int
Aulib::AudioResamplerSpeex::adjustForOutputSpec(int dstRate, int srcRate,
                                                int channels)
{
    int err;
    d->fResampler.reset(speex_resampler_init(channels, srcRate, dstRate, 10, &err));
    if (err != 0) {
        d->fResampler = nullptr;
        return -1;
    }
    return 0;
}
