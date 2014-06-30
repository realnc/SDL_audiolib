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


namespace Aulib {

/// \private
class AudioDecoderWildmidi_priv {
public:
    AudioDecoderWildmidi_priv();
    ~AudioDecoderWildmidi_priv();

    midi* midiHandle;
    Uint8* midiData;
    int midiDataLen;
    Sint16* sampBuf;
    int sampBufLen;
    bool eof;

    static bool initialized;
    static unsigned rate;
};

bool AudioDecoderWildmidi_priv::initialized = false;
unsigned AudioDecoderWildmidi_priv::rate = 0;

} // namespace Aulib



Aulib::AudioDecoderWildmidi_priv::AudioDecoderWildmidi_priv()
    : midiHandle(nullptr),
      midiData(nullptr),
      midiDataLen(0),
      sampBuf(nullptr),
      sampBufLen(0),
      eof(false)
{ }


Aulib::AudioDecoderWildmidi_priv::~AudioDecoderWildmidi_priv()
{
    if (midiHandle) {
        WildMidi_Close(midiHandle);
    }
    delete[] midiData;
    delete[] sampBuf;
}


Aulib::AudioDecoderWildmidi::AudioDecoderWildmidi()
    : d(new Aulib::AudioDecoderWildmidi_priv)
{ }


Aulib::AudioDecoderWildmidi::~AudioDecoderWildmidi()
{
    delete d;
}


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
    if (not AudioDecoderWildmidi_priv::initialized) {
        return false;
    }

    if (d->midiHandle) {
        WildMidi_Close(d->midiHandle);
        delete[] d->midiData;
        d->midiHandle = nullptr;
        d->midiData = nullptr;
    }

    int frontPos = SDL_RWtell(rwops);
    d->midiDataLen = SDL_RWseek(rwops, 0, RW_SEEK_END) - frontPos;
    SDL_RWseek(rwops, frontPos, RW_SEEK_SET);
    if (d->midiDataLen <= 0) {
        return false;
    }

    d->midiData = new Uint8[d->midiDataLen];
    if (SDL_RWread(rwops, d->midiData, d->midiDataLen, 1) != 1
        or (d->midiHandle = WildMidi_OpenBuffer(d->midiData, d->midiDataLen)) == nullptr)
    {
        delete[] d->midiData;
        d->midiData = nullptr;
        return false;
    }
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


int
Aulib::AudioDecoderWildmidi::doDecoding(float buf[], int len, bool& callAgain)
{
    callAgain = false;
    if (d->midiHandle == nullptr or d->eof) {
        return 0;
    }

    if (d->sampBufLen != len) {
        delete[] d->sampBuf;
        d->sampBuf = new Sint16[len];
        d->sampBufLen = len;
    }
    int res = WildMidi_GetOutput(d->midiHandle, (char*)d->sampBuf, len * 2);
    // Convert from 16-bit to float.
    for (int i = 0; i < res / 2; ++i) {
        buf[i] = (float)d->sampBuf[i] / 32768.f;
    }
    if (res < len) {
        d->eof = true;
    }
    if (res < 0) {
        return 0;
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
    if (not d->midiHandle or (info = WildMidi_GetInfo(d->midiHandle)) == nullptr) {
        return -1.f;
    }
    return info->approx_total_samples / AudioDecoderWildmidi_priv::rate;
}


bool
Aulib::AudioDecoderWildmidi::seekToTime(float seconds)
{
    unsigned long samplePos = seconds * AudioDecoderWildmidi_priv::rate;
    return (WildMidi_FastSeek(d->midiHandle, &samplePos) == 0);
}
