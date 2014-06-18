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
#ifndef AUDIORESAMPLER_H
#define AUDIORESAMPLER_H

#include "aulib_global.h"

namespace Aulib {

/*!
 * \brief Abstract base class for audio resamplers.
 *
 * This class receives audio from an AudioDecoder and resamples it to the requested sample
 * rate.
 */
class AULIB_EXPORT AudioResampler {
public:
    /*!
     * \brief Constructs an audio resampler.
     */
    AudioResampler();

    virtual ~AudioResampler();

    /*! \brief Sets the decoder that is to be used as source.
     *
     * \param decoder
     *  The decoder to use as source. Must not be null.
     */
    void setDecoder(class AudioDecoder* decoder);

    /*! \brief Sets the target sample rate, channels and chuck size.
     *
     * \param chunkSize
     *  Specifies how many samples per channel to resample at most in each call to the resample()
     *  function. It is recommended to set this to the same value that was used as buffer size in
     *  the call to Aulib::init().
     */
    int setSpec(unsigned dstRate, unsigned channels, unsigned chunkSize);

    unsigned currentRate() const;
    unsigned currentChannels() const;
    unsigned currentChunkSize() const;

    /*! \brief Fills an output buffer with resampled audio samples.
     *
     * \param dst Output buffer.
     *
     * \return The amount of samples that were stored in the buffer. This can be smaller than
     *         'dstLen' if the decoder has no more samples left.
     */
    int resample(float dst[], int dstLen);

protected:
    /*! \brief Change sample rate and amount of channels.
     *
     * This function must be implemented when subclassing. It is used to notify subclasses about
     * changes in source and target sample rates, as well as the amount of channels in the audio.
     *
     * \param dstRate Target sample rate (rate being resampled to.)
     *
     * \param srcRate Source sample rate (rate being resampled from.)
     *
     * \param channels Amount of channels in both the source as well as the target audio buffers.
     */
    virtual int adjustForOutputSpec(unsigned dstRate, unsigned srcRate, unsigned channels) = 0;

    /*! This function must be implemented when subclassing. It must resample
     * the audio contained in 'src' containing 'srcLen' samples, and store the
     * resulting samples in 'dst', which has a capacity of at most 'dstLen'
     * samples.
     *
     * The 'src' buffer contains audio in either mono or stereo. Stereo is
     * stored in interleaved format.
     *
     * The source and target sample rates, as well as the amount of channels
     * that are to be used must be those that were specified in the last call
     * to the adjustForOutputSpec() function.
     *
     * 'dstLen' and 'srcLen' are both input as well as output parameters. The
     * function must set 'dstLen' to the amount of samples that were actually
     * stored in 'dst', and 'srcLen' to the amount of samples that were
     * actually used from 'src'. For example, if in the following call:
     *
     *      dstLen = 200;
     *      srcLen = 100;
     *      doResampling(dst, src, dstLen, srcLen);
     *
     * the function resamples 98 samples from 'src', resulting in 196 samples
     * which are stored in 'dst', the function must set 'srcLen' to 98 and
     * 'dstLen' to 196.
     *
     * So when implementing this function, you do not need to worry about using
     * up all the available samples in 'src'. Simply resample as much audio
     * from 'src' as you can in order to fill 'dst' as much as possible, and if
     * there's anything left at the end that cannot be resampled, simply ignore
     * it.
     */
    virtual void doResampling(float dst[], const float src[],
                              int& dstLen, int& srcLen) = 0;

private:
    friend class AudioResampler_priv;
    AudioResampler(const AudioResampler&);
    AudioResampler& operator =(const AudioResampler&);

    class AudioResampler_priv* const d;
};

} // namespace Aulib

#endif
