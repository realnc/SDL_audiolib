// This is copyrighted software. More information is at the end of this file.
#include "Aulib/Stream.h"

#include "Aulib/Decoder.h"
#include "Aulib/Processor.h"
#include "Aulib/Resampler.h"
#include "SdlAudioLocker.h"
#include "aulib.h"
#include "aulib_global.h"
#include "aulib_log.h"
#include "missing/algorithm.h"
#include "sampleconv.h"
#include "stream_p.h"
#include <SDL_audio.h>
#include <SDL_timer.h>
#include <mutex>

template <typename T>
Aulib::Stream<T>::Stream(
    const std::string& filename, std::unique_ptr<Decoder<T>> decoder,
    std::unique_ptr<Resampler<T>> resampler)
    : Stream(SDL_RWFromFile(filename.c_str(), "rb"), std::move(decoder), std::move(resampler), true)
{
    if (not d->fRWops) {
        aulib::log::warnLn("Stream failed to create rwops: {}", SDL_GetError());
    }
}

template <typename T>
Aulib::Stream<T>::Stream(const std::string& filename, std::unique_ptr<Decoder<T>> decoder)
    : Stream(SDL_RWFromFile(filename.c_str(), "rb"), std::move(decoder), true)
{
    if (not d->fRWops) {
        aulib::log::warnLn("Stream failed to create rwops: {}", SDL_GetError());
    }
}

template <typename T>
Aulib::Stream<T>::Stream(
    SDL_RWops* rwops, std::unique_ptr<Decoder<T>> decoder, std::unique_ptr<Resampler<T>> resampler,
    bool closeRw)
    : d(std::make_unique<Stream_priv<T>>(
        this, std::move(decoder), std::move(resampler), rwops, closeRw))
{}

template <typename T>
Aulib::Stream<T>::Stream(SDL_RWops* rwops, std::unique_ptr<Decoder<T>> decoder, bool closeRw)
    : d(std::make_unique<Stream_priv<T>>(this, std::move(decoder), nullptr, rwops, closeRw))
{}

template <typename T>
Aulib::Stream<T>::~Stream()
{
    SdlAudioLocker lock;

    d->fStop();
}

template <typename T>
auto Aulib::Stream<T>::open() -> bool
{
    SdlAudioLocker lock;

    if (d->fIsOpen) {
        return true;
    }
    if (not d->fRWops) {
        SDL_SetError("Cannot open stream: null rwops.");
        return false;
    }
    if (not d->fDecoder->open(d->fRWops)) {
        return false;
    }
    if (d->fResampler) {
        d->fResampler->setSpec(Aulib::sampleRate(), Aulib::channelCount(), Aulib::frameSize());
    }
    d->fIsOpen = true;
    return true;
}

template <typename T>
auto Aulib::Stream<T>::play(int iterations, std::chrono::microseconds fadeTime) -> bool
{
    if (not open()) {
        return false;
    }

    SdlAudioLocker locker;

    if (d->fIsPlaying) {
        return true;
    }
    d->fCurrentIteration = 0;
    d->fWantedIterations = iterations;
    d->fPlaybackStartTick = SDL_GetTicks();
    d->fStarting = true;
    if (fadeTime.count() > 0) {
        d->fInternalVolume = 0.f;
        d->fFadingIn = true;
        d->fFadingOut = false;
        d->fFadeInDuration = std::chrono::duration_cast<std::chrono::milliseconds>(fadeTime);
        d->fFadeInStartTick = d->fPlaybackStartTick;
    } else {
        d->fInternalVolume = 1.f;
        d->fFadingIn = false;
    }
    d->fIsPlaying = true;
    {
        std::lock_guard<SdlMutex> lock(d->fStreamListMutex);
        d->fStreamList.push_back(this);
    }
    return true;
}

template <typename T>
void Aulib::Stream<T>::stop(std::chrono::microseconds fadeTime)
{
    SdlAudioLocker lock;

    if (fadeTime.count() > 0) {
        d->fFadingIn = false;
        d->fFadingOut = true;
        d->fFadeOutDuration = std::chrono::duration_cast<std::chrono::milliseconds>(fadeTime);
        d->fFadeOutStartTick = SDL_GetTicks();
        d->fStopAfterFade = true;
    } else {
        d->fStop();
    }
}

template <typename T>
void Aulib::Stream<T>::pause(std::chrono::microseconds fadeTime)
{
    if (not open()) {
        return;
    }

    SdlAudioLocker locker;

    if (d->fIsPaused) {
        return;
    }
    if (fadeTime.count() > 0) {
        d->fFadingIn = false;
        d->fFadingOut = true;
        d->fFadeOutDuration = std::chrono::duration_cast<std::chrono::milliseconds>(fadeTime);
        d->fFadeOutStartTick = SDL_GetTicks();
        d->fStopAfterFade = false;
    } else {
        d->fIsPaused = true;
    }
}

template <typename T>
void Aulib::Stream<T>::resume(std::chrono::microseconds fadeTime)
{
    SdlAudioLocker locker;

    if (not d->fIsPaused) {
        return;
    }
    if (fadeTime.count() > 0) {
        d->fInternalVolume = 0.f;
        d->fFadingIn = true;
        d->fFadingOut = false;
        d->fFadeInDuration = std::chrono::duration_cast<std::chrono::milliseconds>(fadeTime);
        d->fFadeInStartTick = SDL_GetTicks();
    } else {
        d->fInternalVolume = 1.f;
    }
    d->fIsPaused = false;
}

template <typename T>
auto Aulib::Stream<T>::rewind() -> bool
{
    if (not open()) {
        return false;
    }

    SdlAudioLocker locker;
    return d->fDecoder->rewind();
}

template <typename T>
void Aulib::Stream<T>::setVolume(float volume)
{
    SdlAudioLocker locker;

    if (volume < 0.f) {
        volume = 0.f;
    }
    d->fVolume = volume;
}

template <typename T>
auto Aulib::Stream<T>::volume() const -> float
{
    SdlAudioLocker locker;

    return d->fVolume;
}

template <typename T>
void Aulib::Stream<T>::setStereoPosition(const float position)
{
    SdlAudioLocker locker;

    d->fStereoPos = Aulib::priv::clamp(position, -1.f, 1.f);
}

template <typename T>
auto Aulib::Stream<T>::getStereoPosition() const -> float
{
    SdlAudioLocker locker;

    return d->fStereoPos;
}

template <typename T>
void Aulib::Stream<T>::mute()
{
    SdlAudioLocker locker;

    d->fIsMuted = true;
}

template <typename T>
void Aulib::Stream<T>::unmute()
{
    SdlAudioLocker locker;

    d->fIsMuted = false;
}

template <typename T>
auto Aulib::Stream<T>::isMuted() const -> bool
{
    SdlAudioLocker locker;

    return d->fIsMuted;
}

template <typename T>
auto Aulib::Stream<T>::isPlaying() const -> bool
{
    SdlAudioLocker locker;

    return d->fIsPlaying;
}

template <typename T>
auto Aulib::Stream<T>::isPaused() const -> bool
{
    SdlAudioLocker locker;

    return d->fIsPaused;
}

template <typename T>
auto Aulib::Stream<T>::duration() const -> std::chrono::microseconds
{
    SdlAudioLocker locker;

    return d->fDecoder->duration();
}

template <typename T>
auto Aulib::Stream<T>::seekToTime(std::chrono::microseconds pos) -> bool
{
    SdlAudioLocker locker;

    return d->fDecoder->seekToTime(pos);
}

template <typename T>
void Aulib::Stream<T>::setFinishCallback(Callback func)
{
    SdlAudioLocker locker;

    d->fFinishCallback = std::move(func);
}

template <typename T>
void Aulib::Stream<T>::unsetFinishCallback()
{
    SdlAudioLocker locker;

    d->fFinishCallback = nullptr;
}

template <typename T>
void Aulib::Stream<T>::setLoopCallback(Callback func)
{
    SdlAudioLocker locker;

    d->fLoopCallback = std::move(func);
}

template <typename T>
void Aulib::Stream<T>::unsetLoopCallback()
{
    SdlAudioLocker locker;

    d->fLoopCallback = nullptr;
}

template <typename T>
void Aulib::Stream<T>::addProcessor(std::shared_ptr<Processor<T>> processor)
{
    SdlAudioLocker locker;

    if (not processor
        or std::find_if(
               d->processors.begin(), d->processors.end(),
               [&processor](std::shared_ptr<Processor<T>>& p) {
                   return p.get() == processor.get();
               })
               != d->processors.end())
    {
        return;
    }
    d->processors.push_back(std::move(processor));
}

template <typename T>
void Aulib::Stream<T>::removeProcessor(Processor<T>* processor)
{
    SdlAudioLocker locker;

    auto it = std::find_if(
        d->processors.begin(), d->processors.end(),
        [&processor](std::shared_ptr<Processor<T>>& p) { return p.get() == processor; });
    if (it == d->processors.end()) {
        return;
    }
    d->processors.erase(it);
}

template <typename T>
void Aulib::Stream<T>::clearProcessors()
{
    SdlAudioLocker locker;

    d->processors.clear();
}

template <typename T>
void Aulib::Stream<T>::invokeFinishCallback()
{
    if (d->fFinishCallback) {
        d->fFinishCallback(*this);
    }
}

template <typename T>
void Aulib::Stream<T>::invokeLoopCallback()
{
    if (d->fLoopCallback) {
        d->fLoopCallback(*this);
    }
}

template class Aulib::Stream<float>;
template class Aulib::Stream<int32_t>;

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
