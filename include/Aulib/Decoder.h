// This is copyrighted software. More information is at the end of this file.
#pragma once

#include "aulib_export.h"
#include <SDL_stdinc.h>
#include <chrono>
#include <memory>

struct SDL_RWops;

namespace Aulib {

/*!
 * \brief Abstract base class for audio decoders.
 */
class AULIB_EXPORT Decoder
{
public:
    Decoder();
    virtual ~Decoder();

    Decoder(const Decoder&) = delete;
    auto operator=(const Decoder&) -> Decoder& = delete;

    static auto decoderFor(const std::string& filename) -> std::unique_ptr<Decoder>;
    static auto decoderFor(SDL_RWops* rwops) -> std::unique_ptr<Decoder>;

    auto isOpen() const -> bool;
    auto decode(float buf[], int len, bool& callAgain) -> int;

    virtual auto open(SDL_RWops* rwops) -> bool = 0;
    virtual auto getChannels() const -> int = 0;
    virtual auto getRate() const -> int = 0;
    virtual auto rewind() -> bool = 0;
    virtual auto duration() const -> std::chrono::microseconds = 0;
    virtual auto seekToTime(std::chrono::microseconds pos) -> bool = 0;

protected:
    void setIsOpen(bool f);
    virtual auto doDecoding(float buf[], int len, bool& callAgain) -> int = 0;

private:
    const std::unique_ptr<struct Decoder_priv> d;
};

} // namespace Aulib

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
