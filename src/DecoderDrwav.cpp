// This is copyrighted software. More information is at the end of this file.
#include "Aulib/DecoderDrwav.h"

#define DR_WAV_NO_STDIO

#include "aulib_log.h"
#include "dr_wav.h"
#include "missing.h"
#include <SDL_rwops.h>

namespace chrono = std::chrono;

extern "C" {

static auto drwavReadCb(void* const rwops, void* const dst, const size_t len) -> size_t
{
    return SDL_RWread(static_cast<SDL_RWops*>(rwops), dst, 1, len);
}

static auto drwavSeekCb(void* const rwops_void, const int offset, const drwav_seek_origin origin)
    -> drwav_bool32
{
    SDL_ClearError();

    auto* const rwops = static_cast<SDL_RWops*>(rwops_void);
    const auto rwops_size = SDL_RWsize(rwops);
    const auto cur_pos = SDL_RWtell(rwops);

    auto seekIsPastEof = [=] {
        const int abs_offset = offset + (origin == drwav_seek_origin_current ? cur_pos : 0);
        return abs_offset >= rwops_size;
    };

    if (rwops_size < 0) {
        aulib::log::warnLn("dr_wav: Cannot determine rwops size: {}",
                           rwops_size == -1 ? "unknown error" : SDL_GetError());
        return false;
    }
    if (cur_pos < 0) {
        aulib::log::warnLn("dr_wav: Cannot tell current rwops position.");
        return false;
    }

    int whence;
    switch (origin) {
    case drwav_seek_origin_start:
        whence = RW_SEEK_SET;
        break;
    case drwav_seek_origin_current:
        whence = RW_SEEK_CUR;
        break;
    default:
        aulib::log::warnLn("dr_wav: Unrecognized origin in seek callback.");
        return false;
    }
    return not seekIsPastEof() and SDL_RWseek(rwops, offset, whence) >= 0;
}

} // extern "C"

namespace Aulib {

struct DecoderDrwav_priv final
{
    drwav handle_{};
    bool fEOF = false;
};

} // namespace Aulib

namespace {

drwav_uint64 drwavReadPcmFrames(drwav* pWav, drwav_uint64 framesToRead, float* pBufferOut) {
    return drwav_read_pcm_frames_f32(pWav, framesToRead, pBufferOut);
}

drwav_uint64 drwavReadPcmFrames(drwav* pWav, drwav_uint64 framesToRead, int32_t* pBufferOut) {
    return drwav_read_pcm_frames_s32(pWav, framesToRead, pBufferOut);
}

} // namespace

template <typename T>
Aulib::DecoderDrwav<T>::DecoderDrwav()
    : d(std::make_unique<DecoderDrwav_priv>())
{}

template <typename T>
Aulib::DecoderDrwav<T>::~DecoderDrwav()
{
    if (not this->isOpen()) {
        return;
    }
    drwav_uninit(&d->handle_);
}

template <typename T>
auto Aulib::DecoderDrwav<T>::open(SDL_RWops* const rwops) -> bool
{
    if (this->isOpen()) {
        return true;
    }

    if (not drwav_init(&d->handle_, drwavReadCb, drwavSeekCb, rwops, nullptr)) {
        SDL_SetError("drwav_init failed.");
        return false;
    }
    this->setIsOpen(true);
    return true;
}

template <typename T>
auto Aulib::DecoderDrwav<T>::doDecoding(T* const buf, const int len, bool& /*callAgain*/) -> int
{
    if (d->fEOF or not this->isOpen()) {
        return 0;
    }

    const auto ret = drwavReadPcmFrames(&d->handle_, len / getChannels(), buf) * getChannels();
    if (ret < static_cast<drwav_uint64>(len)) {
        d->fEOF = true;
    }
    return ret;
}

template <typename T>
auto Aulib::DecoderDrwav<T>::getChannels() const -> int
{
    return d->handle_.channels;
}

template <typename T>
auto Aulib::DecoderDrwav<T>::getRate() const -> int
{
    return d->handle_.sampleRate;
}

template <typename T>
auto Aulib::DecoderDrwav<T>::rewind() -> bool
{
    return seekToTime({});
}

template <typename T>
auto Aulib::DecoderDrwav<T>::duration() const -> chrono::microseconds
{
    if (not this->isOpen()) {
        return {};
    }
    return chrono::duration_cast<chrono::microseconds>(
        chrono::duration<double>(static_cast<double>(d->handle_.totalPCMFrameCount) / getRate()));
}

template <typename T>
auto Aulib::DecoderDrwav<T>::seekToTime(const chrono::microseconds pos) -> bool
{
    const auto target_frame = chrono::duration<double>(pos).count() * getRate();
    if (not this->isOpen() or not drwav_seek_to_pcm_frame(&d->handle_, target_frame)) {
        return false;
    }
    d->fEOF = false;
    return true;
}

/*

Copyright (C) 2021 Nikos Chantziaras.

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
