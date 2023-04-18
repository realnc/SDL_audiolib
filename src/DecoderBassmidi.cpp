// This is copyrighted software. More information is at the end of this file.
#include "Aulib/DecoderBassmidi.h"

#include "Buffer.h"
#include "aulib.h"
#include "aulib_log.h"
#include "missing.h"
#include <SDL_audio.h>
#include <SDL_rwops.h>
#include <bass.h>
#include <bassmidi.h>
#include <type_traits>

namespace chrono = std::chrono;

static bool bassIsInitialized = false;

class HstreamWrapper final
{
public:
    HstreamWrapper() noexcept = default;

    explicit HstreamWrapper(BOOL mem, const void* file, QWORD offset, QWORD len, DWORD flags,
                            DWORD freq)
    {
        reset(mem, file, offset, len, flags, freq);
    }

    HstreamWrapper(const HstreamWrapper&) = delete;
    HstreamWrapper(HstreamWrapper&&) = delete;
    auto operator=(const HstreamWrapper&) -> HstreamWrapper& = delete;
    auto operator=(HstreamWrapper&&) -> HstreamWrapper& = delete;

    ~HstreamWrapper()
    {
        freeStream();
    }

    explicit operator bool() const noexcept
    {
        return hstream != 0;
    }

    auto get() const noexcept -> HSTREAM
    {
        return hstream;
    }

    void reset(BOOL mem, const void* file, QWORD offset, QWORD len, DWORD flags, DWORD freq)
    {
        freeStream();
        hstream = BASS_MIDI_StreamCreateFile(mem, file, offset, len, flags, freq);
        if (hstream == 0) {
            aulib::log::debugLn("DecoderBassmidi: got BASS error {} while creating HSTREAM.",
                                BASS_ErrorGetCode());
        }
    }

    void swap(HstreamWrapper& other) noexcept
    {
        std::swap(hstream, other.hstream);
    }

private:
    HSTREAM hstream = 0;

    void freeStream() noexcept
    {
        if (hstream == 0) {
            return;
        }
        if (BASS_StreamFree(hstream) == 0) {
            aulib::log::debugLn("DecoderBassmidi: got BASS error {} while freeing HSTREAM.",
                                BASS_ErrorGetCode());
        }
    }
};

namespace Aulib {

struct DecoderBassmidi_priv final
{
    DecoderBassmidi_priv();

    HstreamWrapper hstream;
    Buffer<Uint8> midiData{0};
    bool eof = false;
};

} // namespace Aulib

Aulib::DecoderBassmidi_priv::DecoderBassmidi_priv()
{
    if (bassIsInitialized) {
        return;
    }
    if (BASS_Init(0, Aulib::sampleRate(), 0, nullptr, nullptr) != 0) {
        bassIsInitialized = true;
        return;
    }
    aulib::log::debugLn("DecoderBassmidi: got BASS error {} while initializing.",
                        BASS_ErrorGetCode());
}

template <typename T>
Aulib::DecoderBassmidi<T>::DecoderBassmidi()
    : d(std::make_unique<DecoderBassmidi_priv>())
{}

template <typename T>
Aulib::DecoderBassmidi<T>::~DecoderBassmidi() = default;

template <typename T>
auto Aulib::DecoderBassmidi<T>::setDefaultSoundfont(const std::string& filename) -> bool
{
    if (BASS_SetConfigPtr(BASS_CONFIG_MIDI_DEFFONT, filename.c_str()) != 0) {
        return true;
    }
    aulib::log::debugLn("DecoderBassmidi: got BASS error {} while setting default soundfont.",
                        BASS_ErrorGetCode());
    return false;
}

template <typename T>
auto Aulib::DecoderBassmidi<T>::open(SDL_RWops* rwops) -> bool
{
    if (this->isOpen()) {
        return true;
    }

    // FIXME: error reporting
    Sint64 midiDataLen = SDL_RWsize(rwops);
    if (midiDataLen <= 0) {
        return false;
    }
    Buffer<Uint8> newMidiData(midiDataLen);
    DWORD bassFlags =
        (std::is_same<T, float>::value ? BASS_SAMPLE_FLOAT : 0) | BASS_STREAM_DECODE | BASS_MIDI_DECAYEND | BASS_MIDI_SINCINTER;

    if (SDL_RWread(rwops, newMidiData.get(), newMidiData.size(), 1) != 1) {
        return false;
    }
    d->hstream.reset(TRUE, newMidiData.get(), 0, newMidiData.size(), bassFlags, 1);
    if (not d->hstream) {
        return false;
    }
    d->midiData.swap(newMidiData);
    this->setIsOpen(true);
    return true;
}

template <typename T>
auto Aulib::DecoderBassmidi<T>::getChannels() const -> int
{
    return 2;
}

template <typename T>
auto Aulib::DecoderBassmidi<T>::getRate() const -> int
{
    if (not this->isOpen()) {
        return 0;
    }

    BASS_CHANNELINFO inf;
    if (BASS_ChannelGetInfo(d->hstream.get(), &inf) != 0) {
        return inf.freq;
    }
    aulib::log::debugLn("DecoderBassmidi: got BASS error {} while getting BASS_CHANNELINFO.",
                        BASS_ErrorGetCode());
    return 0;
}

template <typename T>
auto Aulib::DecoderBassmidi<T>::doDecoding(T buf[], int len, bool& /*callAgain*/) -> int
{
    if (d->eof or not this->isOpen()) {
        return 0;
    }

    DWORD byteLen = len * static_cast<int>(sizeof(*buf));
    DWORD ret = BASS_ChannelGetData(
        d->hstream.get(), buf, byteLen | (std::is_same<T, float>::value ? BASS_DATA_FLOAT : 0));
    if (ret == static_cast<DWORD>(-1)) {
        SDL_SetError("DecoderBassmidi: got BASS error %d during decoding.\n", BASS_ErrorGetCode());
        return 0;
    }
    if (ret < byteLen) {
        d->eof = true;
    }
    return ret / static_cast<int>(sizeof(*buf));
}

template <typename T>
auto Aulib::DecoderBassmidi<T>::rewind() -> bool
{
    return seekToTime(chrono::microseconds::zero());
}

template <typename T>
auto Aulib::DecoderBassmidi<T>::duration() const -> chrono::microseconds
{
    if (not this->isOpen()) {
        return {};
    }

    QWORD pos = BASS_ChannelGetLength(d->hstream.get(), BASS_POS_BYTE);
    if (pos == static_cast<QWORD>(-1)) {
        aulib::log::debugLn("DecoderBassmidi: got BASS error {} while getting channel length.",
                            BASS_ErrorGetCode());
        return {};
    }
    double sec = BASS_ChannelBytes2Seconds(d->hstream.get(), pos);
    if (sec < 0) {
        aulib::log::debugLn(
            "DecoderBassmidi: got BASS error {} while translating duration from bytes to "
            "seconds.",
            BASS_ErrorGetCode());
        return {};
    }
    return chrono::duration_cast<chrono::microseconds>(chrono::duration<double>(sec));
}

template <typename T>
auto Aulib::DecoderBassmidi<T>::seekToTime(chrono::microseconds pos) -> bool
{
    if (not this->isOpen()) {
        return false;
    }

    QWORD bytePos =
        BASS_ChannelSeconds2Bytes(d->hstream.get(), chrono::duration<double>(pos).count());
    if (bytePos == static_cast<QWORD>(-1)) {
        SDL_SetError("DecoderBassmidi: got BASS error %d while translating seek time to bytes.",
                     BASS_ErrorGetCode());
        return false;
    }
    if (BASS_ChannelSetPosition(d->hstream.get(), bytePos, BASS_POS_BYTE) == 0) {
        SDL_SetError("DecoderBassmidi: got BASS error %d during rewinding.\n", BASS_ErrorGetCode());
        return false;
    }
    d->eof = false;
    return true;
}

template class Aulib::DecoderBassmidi<float>;
template class Aulib::DecoderBassmidi<int32_t>;

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
