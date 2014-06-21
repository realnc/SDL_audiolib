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
#include "Aulib/AudioStream.h"
#include "audiostream_p.h"

#include <SDL_timer.h>
#include <SDL_audio.h>

#include "aulib_global.h"
#include "aulib.h"
#include "Aulib/AudioDecoder.h"
#include "Aulib/AudioResampler.h"
#include "sampleconv.h"
#include "aulib_debug.h"


Aulib::AudioStream::AudioStream(const char* filename, AudioDecoder* decoder,
                                AudioResampler* resampler)
    : d(new AudioStream_priv(this, decoder, resampler, true))
{
    d->fRWops = SDL_RWFromFile(filename, "rb");
    AM_debugPrintLn(d->fRWops);
}


Aulib::AudioStream::AudioStream(SDL_RWops* rwops, AudioDecoder* decoder, AudioResampler* resampler,
                                bool closeRw)
    : d(new AudioStream_priv(this, decoder, resampler, closeRw))
{
    d->fRWops = rwops;
}


Aulib::AudioStream::~AudioStream()
{
    this->stop();
    delete d;
}


bool
Aulib::AudioStream::open()
{
    if (d->fIsOpen) {
        return true;
    }
    if (d->fRWops == nullptr) {
        return false;
    }
    if (not d->fDecoder->open(d->fRWops)) {
        return false;
    }
    if (d->fResampler) {
        d->fResampler->setSpec(Aulib::spec().freq, Aulib::spec().channels, Aulib::spec().samples);
    }
    d->fIsOpen = true;
    return true;
}


bool
Aulib::AudioStream::play(unsigned iterations, float fadeTime)
{
    if (not open()) {
        return false;
    }
    if (d->fIsPlaying) {
        return true;
    }
    d->fCurrentIteration = 0;
    d->fWantedIterations = iterations;
    d->fPlaybackStartTick = SDL_GetTicks();
    if (fadeTime > 0.f) {
        d->fInternalVolume = 0.f;
        d->fFadingIn = true;
        d->fFadeInTickDuration = fadeTime * 1000.f;
        d->fFadeInStartTick = d->fPlaybackStartTick;
    } else {
        d->fInternalVolume = 1.f;
        d->fFadingIn = false;
    }
    SDL_LockAudio();
    d->fStreamList.push_back(this);
    d->fIsPlaying = true;
    SDL_UnlockAudio();
    return true;
}


void
Aulib::AudioStream::stop(float fadeTime)
{
    if (not d->fIsOpen or not d->fIsPlaying) {
        return;
    }
    SDL_LockAudio();
    if (fadeTime > 0.f) {
        d->fFadingIn = false;
        d->fFadingOut = true;
        d->fFadeOutTickDuration = fadeTime * 1000.f;
        d->fFadeOutStartTick = SDL_GetTicks();
        d->fStopAfterFade = true;
    } else {
        d->fStop();
    }
    SDL_UnlockAudio();
}


void
Aulib::AudioStream::pause(float fadeTime)
{
    if (not open() or d->fIsPaused) {
        return;
    }
    SDL_LockAudio();
    if (fadeTime > 0.f) {
        d->fFadingIn = false;
        d->fFadingOut = true;
        d->fFadeOutTickDuration = fadeTime * 1000.f;
        d->fFadeOutStartTick = SDL_GetTicks();
        d->fStopAfterFade = false;
    } else {
        d->fIsPaused = true;
    }
    SDL_UnlockAudio();
}


void
Aulib::AudioStream::resume(float fadeTime)
{
    if (not d->fIsPaused) {
        return;
    }
    SDL_LockAudio();
    if (fadeTime > 0.f) {
        d->fInternalVolume = 0.f;
        d->fFadingIn = true;
        d->fFadingOut = false;
        d->fFadeInTickDuration = fadeTime * 1000.f;
        d->fFadeInStartTick = SDL_GetTicks();
    } else {
        d->fInternalVolume = 1.f;
    }
    d->fIsPaused = false;
    SDL_UnlockAudio();
}


bool
Aulib::AudioStream::rewind()
{
    if (not open()) {
        return false;
    }
    SDL_LockAudio();
    int ret = d->fDecoder->rewind();
    SDL_UnlockAudio();
    return ret;
}


void
Aulib::AudioStream::setVolume(float volume)
{
    if (volume < 0.f) {
        volume = 0.f;
    }
    SDL_LockAudio();
    d->fVolume = volume;
    SDL_UnlockAudio();
}


float
Aulib::AudioStream::volume() const
{
    return d->fVolume;
}


bool
Aulib::AudioStream::isPlaying() const
{
    return d->fIsPlaying;
}


bool
Aulib::AudioStream::isPaused() const
{
    return d->fIsPaused;
}


float
Aulib::AudioStream::duration() const
{
    return d->fDecoder->duration();
}


bool
Aulib::AudioStream::seekToTime(float seconds)
{
    return d->fDecoder->seekToTime(seconds);
}
