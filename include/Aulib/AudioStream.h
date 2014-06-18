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
#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H

#include <Aulib/Stream.h>

struct SDL_RWops;
struct SDL_AudioSpec;

namespace Aulib {

/*!
 * \brief Sample-based audio playback stream.
 *
 * This class handles playback for sample-based, digital audio streams produced by an AudioDecoder.
 */
class AULIB_EXPORT AudioStream: public Stream {
public:
    /*!
     * \brief Constructs an audio stream from the given file name, decoder and resampler.
     *
     * \param filename
     *  File name from which to feed data to the decoder. Must not be null.
     *
     * \param decoder
     *  Decoder to use for decoding the contents of the file. Must not be null.
     *
     * \param resampler
     *  Resampler to use for converting the sample rate of the audio we get from the decoder. If
     *  this is null, then no resampling will be performed.
     */
    AudioStream(const char* filename, class AudioDecoder* decoder, class AudioResampler* resampler);

    /*!
     * \brief Constructs an audio stream from the given SDL_RWops, decoder and resampler.
     *
     * \param rwops
     *  SDL_RWops from which to feed data to the decoder. Must not be null.
     *
     * \param decoder
     *  Decoder to use for decoding the contents of the SDL_RWops. Must not be null.
     *
     * \param resampler
     *  Resampler to use for converting the sample rate of the audio we get from the decoder. If
     *  this is null, then no resampling will be performed.
     */
    AudioStream(SDL_RWops* rwops, class AudioDecoder* decoder, class AudioResampler* resampler);

    ~AudioStream() override;

    bool open() override;
    bool play(unsigned iterations = 1) override;
    void stop() override;
    void pause() override;
    void resume() override;
    bool rewind() override;
    void setVolume(float volume) override;
    float volume() const override;
    bool isPlaying() const override;
    bool isPaused() const override;
    float duration() const override;
    bool seekToTime(float seconds) override;

private:
    friend class AudioStream_priv;
    friend int Aulib::init(int freq, int format, int channels, int bufferSize);

    class AudioStream_priv* const d;

    AudioStream(const AudioStream&);
    AudioStream& operator =(const AudioStream&);
};

} // namespace Aulib

#endif
