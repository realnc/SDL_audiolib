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
#include "Aulib/AudioResamplerSrc.h"

#include <cstring>
#include <samplerate.h>

namespace Aulib {

/// \private
struct AudioResamplerSRC_priv final {
    std::unique_ptr<SRC_STATE, decltype(&src_delete)> fResampler{nullptr, &src_delete};
    SRC_DATA fData{};
};

} // namespace Aulib


Aulib::AudioResamplerSRC::AudioResamplerSRC()
    : d(std::make_unique<AudioResamplerSRC_priv>())
{ }


Aulib::AudioResamplerSRC::~AudioResamplerSRC()
{ }


void
Aulib::AudioResamplerSRC::doResampling(float dst[], const float src[], int& dstLen, int& srcLen)
{
    if (not d->fResampler) {
        dstLen = srcLen = 0;
        return;
    }

    d->fData.data_in = const_cast<float*>(src);
    d->fData.data_out = dst;
    int channels = currentChannels();
    d->fData.input_frames = srcLen / channels;
    d->fData.output_frames = dstLen / channels;
    d->fData.end_of_input = false;

    src_process(d->fResampler.get(), &d->fData);

    dstLen = d->fData.output_frames_gen * channels;
    srcLen = d->fData.input_frames_used * channels;
}


int
Aulib::AudioResamplerSRC::adjustForOutputSpec(int dstRate, int srcRate, int channels)
{
    int err;
    d->fData.src_ratio = (double)dstRate / (double)srcRate;
    d->fResampler.reset(src_new(SRC_SINC_BEST_QUALITY, channels, &err));
    if (not d->fResampler) {
        return -1;
    }
    return 0;
}
