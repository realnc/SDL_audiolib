// This is copyrighted software. More information is at the end of this file.
#include "Aulib/DecoderOpenmpt.h"

#include "Buffer.h"
#include "aulib.h"
#include "aulib_log.h"
#include "missing.h"
#include <SDL_rwops.h>
#include <libopenmpt/libopenmpt.hpp>
#include <limits>
#include <memory>

namespace chrono = std::chrono;

namespace Aulib {

struct DecoderOpenmpt_priv final
{
    std::unique_ptr<openmpt::module> fModule;
    bool atEOF = false;
    chrono::microseconds fDuration{};
};

} // namespace Aulib

template <typename T>
Aulib::DecoderOpenmpt<T>::DecoderOpenmpt()
    : d(std::make_unique<DecoderOpenmpt_priv>())
{}

template <typename T>
Aulib::DecoderOpenmpt<T>::~DecoderOpenmpt() = default;

template <typename T>
auto Aulib::DecoderOpenmpt<T>::open(SDL_RWops* rwops) -> bool
{
    if (this->isOpen()) {
        return true;
    }
    // FIXME: error reporting
    Sint64 dataSize = SDL_RWsize(rwops);
    if (dataSize <= 0 or dataSize > std::numeric_limits<int>::max()) {
        return false;
    }
    Buffer<Uint8> data(dataSize);
    if (SDL_RWread(rwops, data.get(), data.size(), 1) != 1) {
        return false;
    }

    std::unique_ptr<openmpt::module> module;
    try {
        module = std::make_unique<openmpt::module>(data.get(), data.size());
    }
    catch (const openmpt::exception& e) {
        aulib::log::warnLn("libopenmpt failed to load mod: {}", e.what());
        return false;
    }

    d->fDuration = chrono::duration_cast<chrono::microseconds>(
        chrono::duration<double>(module->get_duration_seconds()));
    d->fModule.swap(module);
    this->setIsOpen(true);
    return true;
}

template <typename T>
auto Aulib::DecoderOpenmpt<T>::getChannels() const -> int
{
    return Aulib::channelCount();
}

template <typename T>
auto Aulib::DecoderOpenmpt<T>::getRate() const -> int
{
    return Aulib::sampleRate();
}

template <typename T>
auto Aulib::DecoderOpenmpt<T>::rewind() -> bool
{
    return seekToTime(chrono::microseconds::zero());
}

template <typename T>
auto Aulib::DecoderOpenmpt<T>::duration() const -> chrono::microseconds
{
    return d->fDuration;
}

template <typename T>
auto Aulib::DecoderOpenmpt<T>::seekToTime(chrono::microseconds pos) -> bool
{
    if (not this->isOpen()) {
        return false;
    }

    d->fModule->set_position_seconds(chrono::duration<double>(pos).count());
    d->atEOF = false;
    return true;
}

template <typename T>
auto Aulib::DecoderOpenmpt<T>::doDecoding(T buf[], int len, bool& /*callAgain*/) -> int
{
    if (d->atEOF or not this->isOpen()) {
        return 0;
    }
    int ret;
    if (Aulib::channelCount() == 2) {
        ret = d->fModule->read_interleaved_stereo(Aulib::sampleRate(), static_cast<size_t>(len / 2),
                                                  buf)
              * 2;
    } else {
        AM_debugAssert(Aulib::channelCount() == 1);
        ret = d->fModule->read(Aulib::sampleRate(), static_cast<size_t>(len), buf);
    }
    if (ret == 0) {
        d->atEOF = true;
    }
    return ret;
}

template class Aulib::DecoderOpenmpt<float>;

// template class Aulib::DecoderOpenmpt<int32_t>;

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
