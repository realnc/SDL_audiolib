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
#include "audiostream_p.h"
#include "audiostream.h"
#include "audioresampler.h"
#include "audiodecoder.h"
#include "aulib_debug.h"


void (*Aulib::AudioStream_priv::fSampleConverter)(Uint8[], float[], int) = nullptr;
SDL_AudioSpec Aulib::AudioStream_priv::fAudioSpec;
std::vector<Aulib::AudioStream*> Aulib::AudioStream_priv::fStreamList;


Aulib::AudioStream_priv::AudioStream_priv(AudioStream* pub, Aulib::AudioDecoder* decoder,
                                          Aulib::AudioResampler* resampler)
    : q(pub),
      fIsOpen(false),
      fDecoder(decoder),
      fResampler(resampler),
      fPreBufferSize(0),
      fIsPlaying(false),
      fIsPaused(false),
      fVolume(1.f),
      fCurrentIteration(0),
      fWantedIterations(0)
{
    if (resampler) {
        resampler->setDecoder(fDecoder);
    }
}


Aulib::AudioStream_priv::~AudioStream_priv()
{
    delete fResampler;
    delete fDecoder;
    if (fRWops) {
        SDL_RWclose(fRWops);
    }
}


void
Aulib::AudioStream_priv::fSdlCallbackImpl(void*, Uint8 out[], int outLen)
{
    AM_debugAssert(AudioStream_priv::fSampleConverter != nullptr);

    int wantedSamples = outLen / ((fAudioSpec.format & 0xFF) / 8);

    static int bufSize = wantedSamples;
    static float* finalMixBuffer = new float[bufSize];
    static float* strmBuf = new float[bufSize];
    if (bufSize != wantedSamples) {
        bufSize = wantedSamples;
        delete[] finalMixBuffer;
        delete[] strmBuf;
        finalMixBuffer = new float[bufSize];
        strmBuf = new float[bufSize];
    }

    // Fill with silence.
    std::fill_n(finalMixBuffer, bufSize, 0.f);

    // Iterate over a copy of the original stream list, since we might want to
    // modify the original as we go, removing streams that have stopped.
    std::vector<AudioStream*> streamList(fStreamList);

    for (size_t i = 0; i < streamList.size(); ++i) {
        AudioStream& stream = *streamList[i];
        if (stream.d->fWantedIterations != 0
            and stream.d->fCurrentIteration >= stream.d->fWantedIterations)
        {
            continue;
        }
        if (stream.d->fIsPaused) {
            continue;
        }
        int len = 0;

        while (len < wantedSamples) {
            if (stream.d->fResampler) {
                len += stream.d->fResampler->resample(strmBuf + len, wantedSamples - len);
            } else {
                bool callAgain = true;
                while (len < wantedSamples and callAgain) {
                    len += stream.d->fDecoder->decode(strmBuf + len, wantedSamples - len,
                                                      callAgain);
                }
            }
            if (len < wantedSamples) {
                stream.d->fDecoder->rewind();
                if (stream.d->fWantedIterations != 0) {
                    ++stream.d->fCurrentIteration;
                    if (stream.d->fCurrentIteration >= stream.d->fWantedIterations) {
                        stream.d->fIsPlaying = false;
                        fStreamList.erase(
                                    std::remove(fStreamList.begin(), fStreamList.end(), &stream),
                                    fStreamList.end());
                        stream.invokeFinishCallback();
                        break;
                    }
                    stream.invokeLoopCallback();
                }
            }
        }

        // Avoid mixing when volume is <= 0.
        if (stream.d->fVolume > 0.f) {
            // Avoid scaling operation when volume is 1.
            if (stream.d->fVolume != 1.f) {
                for (int i = 0; i < len; ++i) {
                    finalMixBuffer[i] += strmBuf[i] * stream.d->fVolume;
                }
            } else {
                for (int i = 0; i < len; ++i) {
                    finalMixBuffer[i] += strmBuf[i];
                }
            }
        }
    }
    AudioStream_priv::fSampleConverter(out, finalMixBuffer, wantedSamples);
}
