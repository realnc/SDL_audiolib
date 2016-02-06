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
#ifndef AUDIODECODERWILDMIDI_H
#define AUDIODECODERWILDMIDI_H

#include <Aulib/AudioDecoder.h>
#include <string>

namespace Aulib {

/*!
 * \brief WildMIDI decoder.
 *
 * \note Before creating any instances of this class, you need to initialize the WildMIDI library
 * by calling the AudioDecoderWildmidi::init() function once.
 */
class AULIB_EXPORT AudioDecoderWildmidi: public AudioDecoder {
public:
    AudioDecoderWildmidi();
    ~AudioDecoderWildmidi() override;

    /*!
     * \brief Initialize the WildMIDI library.
     *
     * This needs to be called before creating any instances of this decoder.
     *
     * \param configFile
     *  Path to the wildmidi.cfg or timidity.cfg configuration file with which to initialize
     *  WildMIDI with.
     *
     * \param rate
     *  Internal WildMIDI sampling rate in Hz. Must be between 11025 and 65000.
     *
     * \param hqResampling
     *  Pass the WM_MO_ENHANCED_RESAMPLING flag to WildMIDI. By default libWildMidi uses linear
     *  interpolation for the resampling of the sound samples. Setting this option enables the
     *  library to use a resampling method that attempts to fill in the gaps giving richer sound.
     *
     * \param reverb
     *  Pass the WM_MO_REVERB flag to WildMIDI. libWildMidi has an 8 reflection reverb engine. Use
     *  this option to give more depth to the output.
     *
     * \return
     *  \retval true WildMIDI was initialized sucessfully.
     *  \retval false WildMIDI could not be initialized.
     */
    static bool init(const std::string configFile, unsigned short rate, bool hqResampling,
                     bool reverb);

    /*!
     * \brief Shut down the WildMIDI library.
     *
     * Shuts down the wildmidi library, resetting data and freeing up memory used by the library.
     *
     * Once this is called, the library is no longer initialized and AudioDecoderWildmidi::init()
     * will need to be called again.
     */
    static void quit();

    bool open(SDL_RWops* rwops) override;
    unsigned getChannels() const override;
    unsigned getRate() const override;
    size_t doDecoding(float buf[], size_t len, bool& callAgain) override;
    bool rewind() override;
    float duration() const override;
    bool seekToTime(float seconds) override;

private:
    class AudioDecoderWildmidi_priv* const d;
};

} // namespace Aulib

#endif
