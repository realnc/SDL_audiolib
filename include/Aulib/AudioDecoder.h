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
#pragma once

#include "aulib_export.h"
#include <SDL_stdinc.h>
#include <memory>

struct SDL_RWops;

namespace Aulib {

/*!
 * \brief Abstract base class for audio decoders.
 */
class AULIB_EXPORT AudioDecoder {
public:
    AudioDecoder();
    virtual ~AudioDecoder();

    AudioDecoder(const AudioDecoder&) = delete;
    AudioDecoder& operator =(const AudioDecoder&) = delete;

    static std::unique_ptr<AudioDecoder> decoderFor(const std::string& filename);
    static std::unique_ptr<AudioDecoder> decoderFor(SDL_RWops* rwops);

    bool isOpen() const;
    int decode(float buf[], int len, bool& callAgain);

    virtual bool open(SDL_RWops* rwops) = 0;
    virtual int getChannels() const = 0;
    virtual int getRate() const = 0;
    virtual bool rewind() = 0;
    virtual float duration() const = 0;
    virtual bool seekToTime(float seconds) = 0;

protected:
    void setIsOpen(bool f);
    virtual int doDecoding(float buf[], int len, bool& callAgain) = 0;

private:
    const std::unique_ptr<struct AudioDecoder_priv> d;
};

} // namespace Aulib
