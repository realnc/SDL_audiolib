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
#include "Aulib/AudioDecoderWildmidi.h"

#include <algorithm>
#include <wildmidi_lib.h>
#include <SDL_rwops.h>
#include "Buffer.h"


namespace Aulib {

/// \private
struct AudioDecoderWildmidi_priv final {
    AudioDecoderWildmidi_priv();

    std::unique_ptr<midi, decltype(&WildMidi_Close)> midiHandle;
    Buffer<Uint8> midiData;
    Buffer<Sint16> sampBuf;
    bool eof;

    static bool initialized;
    static unsigned rate;
};

bool AudioDecoderWildmidi_priv::initialized = false;
unsigned AudioDecoderWildmidi_priv::rate = 0;

} // namespace Aulib



Aulib::AudioDecoderWildmidi_priv::AudioDecoderWildmidi_priv()
    : midiHandle(nullptr, &WildMidi_Close),
      midiData(0),
      sampBuf(0),
      eof(false)
{ }


Aulib::AudioDecoderWildmidi::AudioDecoderWildmidi()
    : d(std::make_unique<Aulib::AudioDecoderWildmidi_priv>())
{ }


Aulib::AudioDecoderWildmidi::~AudioDecoderWildmidi()
{ }


bool
Aulib::AudioDecoderWildmidi::init(const std::string configFile, unsigned short rate,
                                  bool hqResampling, bool reverb)
{
    if (AudioDecoderWildmidi_priv::initialized) {
        return true;
    }
    rate = std::min(std::max(11025u, (unsigned)rate), 65000u);
    AudioDecoderWildmidi_priv::rate = rate;
    unsigned short flags = 0;
    if (hqResampling) {
        flags |= WM_MO_ENHANCED_RESAMPLING;
    }
    if (reverb) {
        flags |= WM_MO_REVERB;
    }
    if (WildMidi_Init(configFile.c_str(), rate, flags) != 0) {
        return false;
    }
    AudioDecoderWildmidi_priv::initialized = true;
    return true;
}


void
Aulib::AudioDecoderWildmidi::quit()
{
    WildMidi_Shutdown();
}


bool
Aulib::AudioDecoderWildmidi::open(SDL_RWops* rwops)
{
    if (isOpen()) {
        return true;
    }
    if (not AudioDecoderWildmidi_priv::initialized) {
        return false;
    }

    //FIXME: error reporting
    Sint64 newMidiDataLen = SDL_RWsize(rwops);
    if (newMidiDataLen <= 0) {
        return false;
    }

    Buffer<Uint8> newMidiData((size_t)newMidiDataLen);
    if (SDL_RWread(rwops, newMidiData.get(), newMidiData.size(), 1) != 1) {
        return false;
    }
    d->midiHandle.reset(WildMidi_OpenBuffer(newMidiData.get(), newMidiData.size()));
    if (not d->midiHandle) {
        return false;
    }
    d->midiData.swap(newMidiData);
    setIsOpen(true);
    return true;
}


unsigned
Aulib::AudioDecoderWildmidi::getChannels() const
{
    return 2;
}


unsigned
Aulib::AudioDecoderWildmidi::getRate() const
{
    return AudioDecoderWildmidi_priv::rate;
}


size_t
Aulib::AudioDecoderWildmidi::doDecoding(float buf[], size_t len, bool& callAgain)
{
    callAgain = false;
    if (not d->midiHandle or d->eof) {
        return 0;
    }

    if (d->sampBuf.size() != len) {
        d->sampBuf.reset(len);
    }
    int res = WildMidi_GetOutput(d->midiHandle.get(), (char*)d->sampBuf.get(), len * 2);
    if (res < 0) {
        return 0;
    }
    // Convert from 16-bit to float.
    for (int i = 0; i < res / 2; ++i) {
        buf[i] = (float)d->sampBuf[i] / 32768.f;
    }
    if ((size_t)res < len) {
        d->eof = true;
    }
    return res / 2;
}


bool
Aulib::AudioDecoderWildmidi::rewind()
{
    if (not d->midiHandle) {
        return false;
    }
    return this->seekToTime(0);
}


float
Aulib::AudioDecoderWildmidi::duration() const
{
    _WM_Info* info;
    if (not d->midiHandle or (info = WildMidi_GetInfo(d->midiHandle.get())) == nullptr) {
        return -1.f;
    }
    return info->approx_total_samples / AudioDecoderWildmidi_priv::rate;
}


bool
Aulib::AudioDecoderWildmidi::seekToTime(float seconds)
{
    unsigned long samplePos = seconds * AudioDecoderWildmidi_priv::rate;
    return (WildMidi_FastSeek(d->midiHandle.get(), &samplePos) == 0);
}
