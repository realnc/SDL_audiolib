// This is copyrighted software. More information is at the end of this file.
#pragma once

#include "Aulib/Processor.h"
#include "Aulib/Stream.h"
#include "Buffer.h"
#include "SdlMutex.h"
#include "aulib.h"
#include <SDL_audio.h>
#include <chrono>
#include <memory>
#include <vector>

namespace Aulib {

template <typename T>
class Decoder;

template <typename T>
class Resampler;

namespace Stream_priv_device {
extern BufferDataType fDataType;
extern ::SDL_AudioSpec fAudioSpec;
#if SDL_VERSION_ATLEAST(2, 0, 0)
extern SDL_AudioDeviceID fDeviceId;
#endif
} // namespace Stream_priv_device

template <typename T>
struct Stream_priv final
{
    const Stream<T>* const q;

    explicit Stream_priv(
        class Stream<T>* pub, std::unique_ptr<Decoder<T>> decoder,
        std::unique_ptr<Resampler<T>> resampler, SDL_RWops* rwops, bool closeRw);
    ~Stream_priv();

    bool fIsOpen = false;
    SDL_RWops* fRWops;
    bool fCloseRw;
    // Resamplers hold a reference to decoders, so we store it as a shared_ptr.
    std::shared_ptr<Decoder<T>> fDecoder;
    std::unique_ptr<Resampler<T>> fResampler;
    bool fIsPlaying = false;
    bool fIsPaused = false;
    float fVolume = 1.f;
    float fStereoPos = 0.f;
    float fInternalVolume = 1.f;
    int fCurrentIteration = 0;
    int fWantedIterations = 0;
    int fPlaybackStartTick = 0;
    int fFadeInStartTick = 0;
    int fFadeOutStartTick = 0;
    bool fStarting = false;
    bool fFadingIn = false;
    bool fFadingOut = false;
    bool fStopAfterFade = false;
    std::chrono::milliseconds fFadeInDuration{};
    std::chrono::milliseconds fFadeOutDuration{};
    std::vector<std::shared_ptr<Processor<T>>> processors;
    bool fIsMuted = false;
    Stream<T>::Callback fFinishCallback;
    Stream<T>::Callback fLoopCallback;

    static std::vector<Stream<T>*> fStreamList;
    static SdlMutex fStreamListMutex;

    // This points to an appropriate converter for the current audio format.
    static void (*fSampleConverter)(Uint8[], const Buffer<T>& src);

    // Sample buffers we use during decoding and mixing.
    static Buffer<T> fFinalMixBuf;
    static Buffer<T> fStrmBuf;
    static Buffer<T> fProcessorBuf;

    auto fProcessFadeAndCheckIfFinished() -> bool;
    void fStop();

    static void fSdlCallbackImpl(void* /*unused*/, Uint8 out[], int outLen);
};

} // namespace Aulib

/*

Copyright (C) 2014, 2015, 2016, 2017, 2018, 2019 Nikos Chantziaras.

This file is part of SDL_audiolib.

SDL_audiolib is free software: you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option) any
later version.

SDL_audiolib is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details.

You should have received a copy of the GNU Lesser General Public License
along with SDL_audiolib. If not, see <http://www.gnu.org/licenses/>.

*/
