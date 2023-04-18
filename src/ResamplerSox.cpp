// This is copyrighted software. More information is at the end of this file.
#include "Aulib/ResamplerSox.h"

#include "aulib_log.h"
#include <cstring>
#include <soxr.h>

namespace Aulib {

template <typename T>
struct ResamplerSox_priv final
{
    explicit ResamplerSox_priv(ResamplerSox<T>::Quality q)
        : fQuality(q)
    {}

    std::unique_ptr<soxr, decltype(&soxr_delete)> fResampler{nullptr, &soxr_delete};
    ResamplerSox<T>::Quality fQuality;
};

} // namespace Aulib

template <typename T>
Aulib::ResamplerSox<T>::ResamplerSox(Quality quality)
    : d(std::make_unique<ResamplerSox_priv<T>>(quality))
{}

template <typename T>
Aulib::ResamplerSox<T>::~ResamplerSox() = default;

template <typename T>
auto Aulib::ResamplerSox<T>::quality() const noexcept -> Aulib::ResamplerSox<T>::Quality
{
    return d->fQuality;
}

template <typename T>
void Aulib::ResamplerSox<T>::doResampling(T dst[], const T src[], int& dstLen, int& srcLen)
{
    if (not d->fResampler) {
        dstLen = srcLen = 0;
        return;
    }
    size_t dstDone, srcDone;
    int channels = this->currentChannels();
    soxr_error_t error;
    error = soxr_process(d->fResampler.get(), src, static_cast<size_t>(srcLen / channels), &srcDone,
                         dst, static_cast<size_t>(dstLen / channels), &dstDone);
    if (error) {
        // FIXME: What do we do?
        aulib::log::warnLn("soxr_process() error: {}", error);
        dstLen = srcLen = 0;
        return;
    }
    dstLen = static_cast<int>(dstDone) * channels;
    srcLen = static_cast<int>(srcDone) * channels;
}

template <typename T>
auto Aulib::ResamplerSox<T>::adjustForOutputSpec(int dstRate, int srcRate, int channels) -> int
{
    soxr_io_spec_t io_spec{};
    io_spec.itype = io_spec.otype = std::is_same<T, float>::value ? SOXR_FLOAT32_I : SOXR_INT32_I;
    io_spec.scale = 1.0;

    const int sox_quality = [&] {
        switch (d->fQuality) {
        case Quality::Quick:
            return SOXR_QQ;
        case Quality::Low:
            return SOXR_LQ;
        case Quality::Medium:
            return SOXR_MQ;
        case Quality::High:
            return SOXR_HQ;
        case Quality::VeryHigh:
            return SOXR_VHQ;
        }
        aulib::log::warnLn(
            "ResamplerSox: Unrecognized ResamplerSox::Quality value {}. Will use Quality::High.",
            static_cast<int>(d->fQuality));
        return SOXR_HQ;
    }();
    auto q_spec = soxr_quality_spec(sox_quality, 0);

    soxr_error_t error;
    d->fResampler.reset(soxr_create(srcRate, dstRate, static_cast<unsigned>(channels), &error,
                                    &io_spec, &q_spec, nullptr));
    if (error) {
        d->fResampler = nullptr;
        return -1;
    }
    return 0;
}

template <typename T>
void Aulib::ResamplerSox<T>::doDiscardPendingSamples()
{
    if (d->fResampler) {
        soxr_clear(d->fResampler.get());
    }
}

template class Aulib::ResamplerSox<float>;
template class Aulib::ResamplerSox<int32_t>;

/*

Copyright (C) 2014, 2015, 2016, 2017, 2018, 2019 Nikos Chantziaras.

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
