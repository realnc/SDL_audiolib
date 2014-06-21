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

#include <vector>
#include <queue>
#include <SDL_audio.h>

namespace Aulib {

/// \private
class AudioStream_priv {
private:
    friend class AudioStream;
    friend int init(int freq, int format, int channels, int bufferSize);
    friend void Aulib::quit();
    friend const SDL_AudioSpec& Aulib::spec();
    class AudioStream* q;

    AudioStream_priv(class AudioStream* pub, class AudioDecoder *decoder,
                     class AudioResampler *resampler);
    ~AudioStream_priv();

    bool fIsOpen;
    SDL_RWops* fRWops;
    class AudioDecoder* fDecoder;
    class AudioResampler* fResampler;
    float* fPreBuffer;
    int fPreBufferSize;
    bool fIsPlaying;
    bool fIsPaused;
    float fVolume;
    float fInternalVolume;
    unsigned fCurrentIteration;
    unsigned fWantedIterations;
    unsigned fPlaybackStartTick;
    unsigned fFadeInStartTick;
    unsigned fFadeOutStartTick;
    bool fFadingIn;
    bool fFadingOut;
    bool fStopAfterFade;
    Uint32 fFadeInTickDuration;
    Uint32 fFadeOutTickDuration;

    static ::SDL_AudioSpec fAudioSpec;
    static std::vector<AudioStream*> fStreamList;

    // This points to an appropriate converter for the current audio format.
    static void (*fSampleConverter)(Uint8[], float[], int);

    void fProcessFade();
    void fStop();

public:
    static void fSdlCallbackImpl(void*, Uint8 out[], int outLen);
};

} // namespace Aulib

#endif
