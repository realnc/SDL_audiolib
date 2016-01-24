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
#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include <SDL_stdinc.h>
#include "aulib_global.h"

struct SDL_RWops;

namespace Aulib {

/*!
 * \brief Abstract base class for audio decoders.
 */
class AULIB_EXPORT AudioDecoder {
public:
    AudioDecoder();
    virtual ~AudioDecoder();

    static AudioDecoder* decoderFor(const char* filename);
    static AudioDecoder* decoderFor(SDL_RWops* rwops);

    bool isOpen() const;
    int decode(float buf[], int len, bool& callAgain);

    virtual bool open(SDL_RWops* rwops) = 0;
    virtual unsigned getChannels() const = 0;
    virtual unsigned getRate() const = 0;
    virtual bool rewind() = 0;
    virtual float duration() const = 0;
    virtual bool seekToTime(float seconds) = 0;

protected:
    void setIsOpen(bool f);

    virtual int doDecoding(float buf[], int len, bool& callAgain) = 0;

private:
    AudioDecoder(const AudioDecoder&);
    AudioDecoder& operator =(const AudioDecoder&);

    class AudioDecoder_priv* const d;
};

} // namespace Aulib

#endif
