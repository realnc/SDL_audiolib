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

#include <algorithm>
#include <cmath>
#include <SDL_timer.h>

#include "Aulib/AudioStream.h"
#include "Aulib/AudioResampler.h"
#include "Aulib/AudioDecoder.h"
#include "aulib_debug.h"


void (*Aulib::AudioStream_priv::fSampleConverter)(Uint8[], float[], int) = nullptr;
SDL_AudioSpec Aulib::AudioStream_priv::fAudioSpec;
std::vector<Aulib::AudioStream*> Aulib::AudioStream_priv::fStreamList;
float* Aulib::AudioStream_priv::fFinalMixBuf = nullptr;
float* Aulib::AudioStream_priv::fStrmBuf = nullptr;
int Aulib::AudioStream_priv::fBufLen = 0;


Aulib::AudioStream_priv::AudioStream_priv(AudioStream* pub, Aulib::AudioDecoder* decoder,
                                          Aulib::AudioResampler* resampler, bool closeRw)
    : q(pub),
      fIsOpen(false),
      fCloseRw(closeRw),
      fDecoder(decoder),
      fResampler(resampler),
      fIsPlaying(false),
      fIsPaused(false),
      fVolume(1.f),
      fInternalVolume(1.f),
      fCurrentIteration(0),
      fWantedIterations(0),
      fPlaybackStartTick(0),
      fFadeInStartTick(0),
      fFadeOutStartTick(0),
      fFadingIn(false),
      fFadingOut(false),
      fStopAfterFade(false),
      fFadeInTickDuration(0),
      fFadeOutTickDuration(0)
{
    if (resampler) {
        resampler->setDecoder(fDecoder);
    }
}


Aulib::AudioStream_priv::~AudioStream_priv()
{
    delete fResampler;
    delete fDecoder;
    if (fCloseRw and fRWops) {
        SDL_RWclose(fRWops);
    }
}


void
Aulib::AudioStream_priv::fProcessFade()
{

    if (fFadingIn) {
        Uint32 now = SDL_GetTicks();
        Uint32 curPos = now - fFadeInStartTick;
        if (curPos >= fFadeInTickDuration) {
            fInternalVolume = 1.f;
            fFadingIn = false;
            return;
        }
        fInternalVolume = std::pow((float)(now - fFadeInStartTick) / fFadeInTickDuration, 3.f);
    } else if (fFadingOut) {
        Uint32 now = SDL_GetTicks();
        Uint32 curPos = now - fFadeOutStartTick;
        if (curPos >= fFadeOutTickDuration) {
            fInternalVolume = 0.f;
            fFadingIn = false;
            if (fStopAfterFade) {
                fStopAfterFade = false;
                fStop();
            } else {
                fIsPaused = true;
            }
            return;
        }
        fInternalVolume = std::pow(-(float)(now - fFadeOutStartTick) / fFadeOutTickDuration + 1.f,
                                   3.f);
    }
}


void
Aulib::AudioStream_priv::fStop()
{
    fStreamList.erase(std::remove(fStreamList.begin(), fStreamList.end(), this->q),
                      fStreamList.end());
    fDecoder->rewind();
    fIsPlaying = false;
}


void
Aulib::AudioStream_priv::fSdlCallbackImpl(void*, Uint8 out[], int outLen)
{
    AM_debugAssert(AudioStream_priv::fSampleConverter != nullptr);

    int wantedSamples = outLen / ((fAudioSpec.format & 0xFF) / 8);

    if (fBufLen != wantedSamples) {
        fBufLen = wantedSamples;
        delete[] fFinalMixBuf;
        delete[] fStrmBuf;
        fFinalMixBuf = new float[fBufLen];
        fStrmBuf = new float[fBufLen];
    }

    // Fill with silence.
    std::fill_n(fFinalMixBuf, fBufLen, 0.f);

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
                len += stream.d->fResampler->resample(fStrmBuf + len, wantedSamples - len);
            } else {
                bool callAgain = true;
                while (len < wantedSamples and callAgain) {
                    len += stream.d->fDecoder->decode(fStrmBuf + len, wantedSamples - len,
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

        stream.d->fProcessFade();
        float volume = stream.d->fVolume * stream.d->fInternalVolume;

        // Avoid mixing on zero volume.
        if (volume > 0.f) {
            // Avoid scaling operation when volume is 1.
            if (volume != 1.f) {
                for (int i = 0; i < len; ++i) {
                    fFinalMixBuf[i] += fStrmBuf[i] * volume;
                }
            } else {
                for (int i = 0; i < len; ++i) {
                    fFinalMixBuf[i] += fStrmBuf[i];
                }
            }
        }
    }
    AudioStream_priv::fSampleConverter(out, fFinalMixBuf, wantedSamples);
}
