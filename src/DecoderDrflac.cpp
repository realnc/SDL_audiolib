// This is copyrighted software. More information is at the end of this file.
#include "Aulib/DecoderDrflac.h"

#include "aulib_debug.h"
#include "missing.h"
#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"
#include <SDL_rwops.h>

namespace chrono = std::chrono;

extern "C" {

static auto drflacReadCb(void* const rwops, void* const dst, const size_t len) -> size_t
{
    return SDL_RWread(static_cast<SDL_RWops*>(rwops), dst, 1, len);
}

static auto drflacSeekCb(void* const rwops_void, const int offset, const drflac_seek_origin origin)
    -> drflac_bool32
{
    SDL_ClearError();

    auto* const rwops = static_cast<SDL_RWops*>(rwops_void);
    const auto rwops_size = SDL_RWsize(rwops);
    const auto cur_pos = SDL_RWtell(rwops);

    auto seekIsPastEof = [=] {
        const int abs_offset = offset + (origin == drflac_seek_origin_current ? cur_pos : 0);
        return abs_offset >= rwops_size;
    };

    if (rwops_size < 0) {
        AM_warnLn("dr_flac: Cannot determine rwops size: " << (rwops_size == -1 ? "unknown error"
                                                                                : SDL_GetError()));
        return false;
    }
    if (cur_pos < 0) {
        AM_warnLn("dr_flac: Cannot tell current rwops position.");
        return false;
    }

    int whence;
    switch (origin) {
    case drflac_seek_origin_start:
        whence = RW_SEEK_SET;
        break;
    case drflac_seek_origin_current:
        whence = RW_SEEK_CUR;
        break;
    default:
        AM_warnLn("dr_flac: Unrecognized origin in seek callback.");
        return false;
    }
    return not seekIsPastEof() and SDL_RWseek(rwops, offset, whence) >= 0;
}

} // extern "C"

namespace Aulib {

struct DecoderFlac_priv final
{
    std::unique_ptr<drflac, decltype(&drflac_close)> handle_{nullptr, drflac_close};
    bool fEOF = false;
};

} // namespace Aulib

Aulib::DecoderDrflac::DecoderDrflac()
    : d(std::make_unique<DecoderFlac_priv>())
{}

Aulib::DecoderDrflac::~DecoderDrflac() = default;

auto Aulib::DecoderDrflac::open(SDL_RWops* const rwops) -> bool
{
    if (isOpen()) {
        return true;
    }

    d->handle_ = {drflac_open(drflacReadCb, drflacSeekCb, rwops, nullptr), drflac_close};
    if (!d->handle_) {
        SDL_SetError("drflac_open returned null.");
        return false;
    }
    setIsOpen(true);
    return true;
}

auto Aulib::DecoderDrflac::doDecoding(float* const buf, const int len, bool& callAgain) -> int
{
    callAgain = false;
    if (d->fEOF) {
        return 0;
    }

    const auto ret =
        drflac_read_pcm_frames_f32(d->handle_.get(), len / getChannels(), buf) * getChannels();
    if (ret < static_cast<drflac_uint64>(len)) {
        d->fEOF = true;
    }
    return ret;
}

auto Aulib::DecoderDrflac::getChannels() const -> int
{
    return d->handle_.get()->channels;
}

auto Aulib::DecoderDrflac::getRate() const -> int
{
    return d->handle_.get()->sampleRate;
}

auto Aulib::DecoderDrflac::rewind() -> bool
{
    if (drflac_seek_to_pcm_frame(d->handle_.get(), 0)) {
        d->fEOF = false;
        return true;
    }
    return false;
}

auto Aulib::DecoderDrflac::duration() const -> chrono::microseconds
{
    return chrono::duration_cast<chrono::microseconds>(chrono::duration<double>(
        static_cast<double>(d->handle_.get()->totalPCMFrameCount) / getRate()));
}

auto Aulib::DecoderDrflac::seekToTime(const chrono::microseconds pos) -> bool
{
    const auto target_frame = chrono::duration<double>(pos).count() * getRate();
    if (!drflac_seek_to_pcm_frame(d->handle_.get(), target_frame)) {
        return false;
    }
    d->fEOF = false;
    return true;
}

/*

Copyright (C) 2020 Nikos Chantziaras.

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
