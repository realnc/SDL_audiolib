// This is copyrighted software. More information is at the end of this file.
#include "Aulib/DecoderOpus.h"

#include "aulib_log.h"
#include <SDL_rwops.h>
#include <cstring>
#include <opusfile.h>

namespace chrono = std::chrono;

extern "C" {

static auto opusReadCb(void* rwops, unsigned char* ptr, int nbytes) -> int
{
    return SDL_RWread(static_cast<SDL_RWops*>(rwops), ptr, 1, nbytes);
}

static auto opusSeekCb(void* rwops, opus_int64 offset, int whence) -> int
{
    if (SDL_RWseek(static_cast<SDL_RWops*>(rwops), offset, whence) < 0) {
        return -1;
    }
    return 0;
}

static auto opusTellCb(void* rwops) -> opus_int64
{
    return SDL_RWtell(static_cast<SDL_RWops*>(rwops));
}

} // extern "C"

namespace Aulib {

struct DecoderOpus_priv final
{
    std::unique_ptr<OggOpusFile, decltype(&op_free)> fOpusHandle{nullptr, &op_free};
    OpusFileCallbacks fCbs{opusReadCb, opusSeekCb, opusTellCb, nullptr};
    bool fEOF = false;
    chrono::microseconds fDuration{};
};

} // namespace Aulib

template <typename T>
Aulib::DecoderOpus<T>::DecoderOpus()
    : d(std::make_unique<DecoderOpus_priv>())
{}

template <typename T>
Aulib::DecoderOpus<T>::~DecoderOpus() = default;

template <typename T>
auto Aulib::DecoderOpus<T>::open(SDL_RWops* rwops) -> bool
{
    if (this->isOpen()) {
        return true;
    }
    int error;
    d->fOpusHandle.reset(op_open_callbacks(rwops, &d->fCbs, nullptr, 0, &error));
    if (not d->fOpusHandle) {
        aulib::log::debugLn("ERROR: {}", error);
        if (error == OP_ENOTFORMAT) {
            aulib::log::debugLn("OP_ENOTFORMAT");
        }
        return false;
    }
    ogg_int64_t len = op_pcm_total(d->fOpusHandle.get(), -1);
    if (len == OP_EINVAL) {
        d->fDuration = chrono::microseconds::zero();
    } else {
        // Opus is always 48kHz.
        d->fDuration =
            chrono::duration_cast<chrono::microseconds>(chrono::duration<double>(len / 48000.));
    }
    this->setIsOpen(true);
    return true;
}

template <typename T>
auto Aulib::DecoderOpus<T>::getChannels() const -> int
{
    return 2;
}

template <typename T>
auto Aulib::DecoderOpus<T>::getRate() const -> int
{
    return 48000;
}

template <typename T>
auto Aulib::DecoderOpus<T>::doDecoding(T buf[], int len, bool& /*callAgain*/) -> int
{
    if (d->fEOF or not this->isOpen()) {
        return 0;
    }

    int decSamples = 0;

    while (decSamples < len) {
        int ret = op_read_float_stereo(d->fOpusHandle.get(), reinterpret_cast<float *>(buf) + decSamples, len - decSamples);
        if (ret == 0) {
            d->fEOF = true;
            break;
        }
        if (ret < 0) {
            aulib::log::debug("libopusfile stream error: ");
            switch (ret) {
            case OP_HOLE:
                aulib::log::debugLn("OP_HOLE");
                break;
            case OP_EBADLINK:
                aulib::log::debugLn("OP_EBADLINK");
                break;
            case OP_EINVAL:
                aulib::log::debugLn("OP_EINVAL");
                break;
            default:
                aulib::log::debugLn("unknown error: {}", ret);
            }
            break;
        }
        decSamples += ret * 2;
    }
    if (std::is_same<int32_t, T>::value) {
        // Convert float to int32_t.
        for (size_t i = 0; i < decSamples; ++i) {
            buf[i] = reinterpret_cast<float *>(buf)[i] * 32768.f;
        }
    }
    return decSamples;
}

template <typename T>
auto Aulib::DecoderOpus<T>::rewind() -> bool
{
    if (not this->isOpen() or op_raw_seek(d->fOpusHandle.get(), 0) != 0) {
        return false;
    }
    d->fEOF = false;
    return true;
}

template <typename T>
auto Aulib::DecoderOpus<T>::duration() const -> chrono::microseconds
{
    return d->fDuration;
}

template <typename T>
auto Aulib::DecoderOpus<T>::seekToTime(chrono::microseconds pos) -> bool
{
    if (not this->isOpen()
        or op_pcm_seek(d->fOpusHandle.get(), chrono::duration<double>(pos).count() * 48000) != 0)
    {
        return false;
    }
    d->fEOF = false;
    return true;
}

template class Aulib::DecoderOpus<float>;
template class Aulib::DecoderOpus<int32_t>;

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
