// This is copyrighted software. More information is at the end of this file.
#include "stream_p.h"

#include "Aulib/Decoder.h"
#include "Aulib/Resampler.h"
#include "Aulib/Stream.h"
#include "aulib_debug.h"
#include "aulib_log.h"
#include "missing.h"
#include <SDL_timer.h>
#include <algorithm>
#include <cmath>
#include <mutex>
#include <type_traits>

template <typename T>
void (*Aulib::Stream_priv<T>::fSampleConverter)(Uint8[], const Buffer<T>& src) = nullptr;

Aulib::BufferDataType Aulib::Stream_priv_device::fDataType;

SDL_AudioSpec Aulib::Stream_priv_device::fAudioSpec;

#if SDL_VERSION_ATLEAST(2, 0, 0)
SDL_AudioDeviceID Aulib::Stream_priv_device::fDeviceId;
#endif

template <typename T>
std::vector<Aulib::Stream<T>*> Aulib::Stream_priv<T>::fStreamList;

template <typename T>
SdlMutex Aulib::Stream_priv<T>::fStreamListMutex;

template <typename T>
Buffer<T> Aulib::Stream_priv<T>::fFinalMixBuf{0};

template <typename T>
Buffer<T> Aulib::Stream_priv<T>::fStrmBuf{0};

template <typename T>
Buffer<T> Aulib::Stream_priv<T>::fProcessorBuf{0};

template <typename T>
Aulib::Stream_priv<T>::Stream_priv(
    Stream<T>* pub, std::unique_ptr<Decoder<T>> decoder, std::unique_ptr<Resampler<T>> resampler,
    SDL_RWops* rwops, bool closeRw)
    : q(pub)
    , fRWops(rwops)
    , fCloseRw(closeRw)
    , fDecoder(std::move(decoder))
    , fResampler(std::move(resampler))
{
    if (fResampler) {
        fResampler->setDecoder(fDecoder);
    }
}

template <typename T>
Aulib::Stream_priv<T>::~Stream_priv()
{
    if (fCloseRw and fRWops) {
        SDL_RWclose(fRWops);
    }
}

template <typename T>
auto Aulib::Stream_priv<T>::fProcessFadeAndCheckIfFinished() -> bool
{
    static_assert(std::is_same<decltype(fFadeInDuration), std::chrono::milliseconds>::value, "");
    static_assert(std::is_same<decltype(fFadeOutDuration), std::chrono::milliseconds>::value, "");

    if (fFadingIn) {
        Sint64 now = SDL_GetTicks();
        Sint64 curPos = now - fFadeInStartTick;
        if (curPos >= fFadeInDuration.count()) {
            fInternalVolume = 1.f;
            fFadingIn = false;
            return false;
        }
        fInternalVolume =
            std::pow(static_cast<float>(now - fFadeInStartTick) / fFadeInDuration.count(), 3.f);
    } else if (fFadingOut) {
        Sint64 now = SDL_GetTicks();
        Sint64 curPos = now - fFadeOutStartTick;
        if (curPos >= fFadeOutDuration.count()) {
            fInternalVolume = 0.f;
            fFadingIn = false;
            if (fStopAfterFade) {
                fStopAfterFade = false;
                fStop();
                return true;
            }
            fIsPaused = true;
            return false;
        }
        fInternalVolume = std::pow(
            -static_cast<float>(now - fFadeOutStartTick) / fFadeOutDuration.count() + 1.f, 3.f);
    }
    return false;
}

template <typename T>
void Aulib::Stream_priv<T>::fStop()
{
    {
        std::lock_guard<SdlMutex> lock(fStreamListMutex);
        fStreamList.erase(std::remove(fStreamList.begin(), fStreamList.end(), this->q),
                          fStreamList.end());
    }
    fDecoder->rewind();
    fIsPlaying = false;
}

template <typename T>
void Aulib::Stream_priv<T>::fSdlCallbackImpl(void* /*unused*/, Uint8 out[], int outLen)
{
    AM_debugAssert(Stream_priv<T>::fSampleConverter);
    const auto& fAudioSpec = Aulib::Stream_priv_device::fAudioSpec;

    const int out_len_samples = outLen / (SDL_AUDIO_BITSIZE(fAudioSpec.format) / 8);
    const int out_len_frames = out_len_samples / fAudioSpec.channels;

    if (fStrmBuf.size() != out_len_samples) {
        fFinalMixBuf.reset(out_len_samples);
        fStrmBuf.reset(out_len_samples);
        fProcessorBuf.reset(out_len_samples);
    }

    // Fill with silence.
    std::fill(fFinalMixBuf.begin(), fFinalMixBuf.end(), 0.f);

    // Iterate over a copy of the original stream list, since we might want to
    // modify the original as we go, removing streams that have stopped.
    const std::vector<Stream<T>*> streamList = [] {
        std::lock_guard<SdlMutex> lock(fStreamListMutex);
        return fStreamList;
    }();

    const int now_tick = SDL_GetTicks();
    const int wanted_ticks = out_len_frames * 1000 / fAudioSpec.freq;

    for (const auto stream : streamList) {
        if (stream->d->fWantedIterations != 0
            and stream->d->fCurrentIteration >= stream->d->fWantedIterations) {
            continue;
        }
        if (stream->d->fIsPaused) {
            continue;
        }

        const int ticks_since_play_start = now_tick - stream->d->fPlaybackStartTick;
        if (ticks_since_play_start <= 0) {
            continue;
        }

        bool has_finished = false;
        bool has_looped = false;
        const int out_offset = [&] {
            if (!stream->d->fStarting || ticks_since_play_start >= wanted_ticks) {
                return 0;
            }

            const int out_offset_ticks = wanted_ticks - ticks_since_play_start;
            const int offset = out_offset_ticks * fAudioSpec.channels * fAudioSpec.freq / 1000;
            return offset - (offset % fAudioSpec.channels);
        }();
        int cur_pos = out_offset;
        stream->d->fStarting = false;

        while (cur_pos < out_len_samples) {
            if (stream->d->fResampler) {
                cur_pos += stream->d->fResampler->resample(fStrmBuf.get() + cur_pos,
                                                           out_len_samples - cur_pos);
            } else {
                bool callAgain = false;
                do {
                    callAgain = false;
                    cur_pos += stream->d->fDecoder->decode(fStrmBuf.get() + cur_pos,
                                                           out_len_samples - cur_pos, callAgain);
                } while (cur_pos < out_len_samples and callAgain);
            }
            for (const auto& proc : stream->d->processors) {
                const int len = cur_pos - out_offset;
                proc->process(fProcessorBuf.get() + out_offset, fStrmBuf.get() + out_offset, len);
                std::memcpy(fStrmBuf.get() + out_offset, fProcessorBuf.get() + out_offset,
                            len * sizeof(*fStrmBuf.get()));
            }
            if (cur_pos < out_len_samples) {
                stream->d->fDecoder->rewind();
                if (stream->d->fWantedIterations != 0) {
                    ++stream->d->fCurrentIteration;
                    if (stream->d->fCurrentIteration >= stream->d->fWantedIterations) {
                        stream->d->fIsPlaying = false;
                        {
                            std::lock_guard<SdlMutex> lock(fStreamListMutex);
                            fStreamList.erase(
                                std::remove(fStreamList.begin(), fStreamList.end(), stream),
                                fStreamList.end());
                        }
                        has_finished = true;
                        break;
                    }
                    has_looped = true;
                }
            }
        }

        has_finished |= stream->d->fProcessFadeAndCheckIfFinished();

        // TODO: mix volume in int64 space for int32 buffers.
        float volumeLeft = stream->d->fVolume * stream->d->fInternalVolume;
        float volumeRight = stream->d->fVolume * stream->d->fInternalVolume;

        if (fAudioSpec.channels > 1) {
            if (stream->d->fStereoPos < 0.f) {
                volumeRight *= 1.f + stream->d->fStereoPos;
            } else if (stream->d->fStereoPos > 0.f) {
                volumeLeft *= 1.f - stream->d->fStereoPos;
            }
        }

        // Avoid mixing on zero volume.
        if (not stream->d->fIsMuted and (volumeLeft > 0.f or volumeRight > 0.f)) {
            // Avoid scaling operation when volume is 1.
            if (fAudioSpec.channels > 1 and (volumeLeft != 1.f or volumeRight != 1.f)) {
                for (int i = out_offset; i < cur_pos; i += 2) {
                    fFinalMixBuf[i] += fStrmBuf[i] * volumeLeft;
                    fFinalMixBuf[i + 1] += fStrmBuf[i + 1] * volumeRight;
                }
            } else if (volumeLeft != 1.f) {
                for (int i = out_offset; i < cur_pos; ++i) {
                    fFinalMixBuf[i] += fStrmBuf[i] * volumeLeft;
                }
            } else {
                for (int i = out_offset; i < cur_pos; ++i) {
                    fFinalMixBuf[i] += fStrmBuf[i];
                }
            }
        }

        if (has_finished) {
            stream->invokeFinishCallback();
        } else if (has_looped) {
            stream->invokeLoopCallback();
        }
    }
    Stream_priv<T>::fSampleConverter(out, fFinalMixBuf);
}

template struct Aulib::Stream_priv<float>;
template struct Aulib::Stream_priv<int32_t>;

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
