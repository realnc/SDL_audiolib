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
#ifndef DEC_BASSMIDI_H
#define DEC_BASSMIDI_H

#include <Aulib/AudioDecoder.h>

namespace Aulib {

/*!
 * \brief BASSMIDI decoder.
 */
class AULIB_EXPORT AudioDecoderBassmidi: public AudioDecoder {
public:
    AudioDecoderBassmidi();
    ~AudioDecoderBassmidi() override;

    /*!
     * \brief Set the default soundfont.
     *
     *  The soundfont will be used for all subsequently opened instances. Instances for which open()
     *  has been called already are not affected.
     *
     * \param filename The soundfont file to use.
     * \return
     *  \retval true The default soundfont was successfully set.
     *  \retval false An error occured.
     */
    static bool setDefaultSoundfont(const char filename[]);

    bool open(SDL_RWops* rwops) override;
    unsigned getChannels() const override;
    unsigned getRate() const override;
    size_t doDecoding(float buf[], size_t len, bool& callAgain) override;
    bool rewind() override;
    float duration() const override;
    bool seekToTime(float seconds) override;

private:
    const std::unique_ptr<struct AudioDecoderBassmidi_priv> d;
};

} // namespace Aulib

#endif
