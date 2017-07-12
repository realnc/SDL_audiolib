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
#ifndef AUDIOSTREAM_P_H
#define AUDIOSTREAM_P_H

#include <memory>
#include <vector>
#include <queue>
#include <SDL_audio.h>
#include <aulib.h>

namespace Aulib {

/// \private
struct AudioStream_priv final {
    friend class AudioStream;
    friend int init(int freq, SDL_AudioFormat format, int channels, int bufferSize);
    friend void Aulib::quit();
    friend const SDL_AudioSpec& Aulib::spec();
    const class AudioStream* const q;

    explicit AudioStream_priv(class AudioStream* pub, class AudioDecoder *decoder,
                              class AudioResampler *resampler, bool closeRw);
    ~AudioStream_priv();

    bool fIsOpen;
    SDL_RWops* fRWops;
    bool fCloseRw;
    std::unique_ptr<AudioDecoder> fDecoder;
    std::unique_ptr<AudioResampler> fResampler;
    bool fIsPlaying;
    bool fIsPaused;
    float fVolume;
    float fInternalVolume;
    int fCurrentIteration;
    int fWantedIterations;
    int fPlaybackStartTick;
    int fFadeInStartTick;
    int fFadeOutStartTick;
    bool fFadingIn;
    bool fFadingOut;
    bool fStopAfterFade;
    int fFadeInTickDuration;
    int fFadeOutTickDuration;

    static ::SDL_AudioSpec fAudioSpec;
    static std::vector<AudioStream*> fStreamList;

    // This points to an appropriate converter for the current audio format.
    static void (*fSampleConverter)(Uint8[], float[], int);

    // Sample buffers we use during decoding and mixing.
    static float* fFinalMixBuf;
    static float* fStrmBuf;
    static int fBufLen;

    void fProcessFade();
    void fStop();

    static void fSdlCallbackImpl(void*, Uint8 out[], int outLen);
};

} // namespace Aulib

#endif
