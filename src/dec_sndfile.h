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
#ifndef DEC_SNDFILE_H
#define DEC_SNDFILE_H

#include "audiodecoder.h"

namespace Aulib {

/*!
 * \brief Libsndfile decoder.
 */
class AULIB_EXPORT AudioDecoderSndfile: public AudioDecoder {
public:
    AudioDecoderSndfile();
    ~AudioDecoderSndfile() override;

    bool open(SDL_RWops* rwops) override;
    unsigned getChannels() const override;
    unsigned getRate() const override;
    int doDecoding(float buf[], int len, bool& callAgain) override;
    bool rewind() override;
    float duration() const override;
    bool seekToTime(float seconds) override;

private:
    friend class AudioDecoderSndfile_priv;
    class AudioDecoderSndfile_priv* const d;
};

} // namespace Aulib

#endif
