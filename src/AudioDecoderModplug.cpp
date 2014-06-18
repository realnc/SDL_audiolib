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
#include "Aulib/AudioDecoderModplug.h"
#include "aulib_global.h"

#include <libmodplug/modplug.h>
#include <SDL_audio.h>

#include "aulib.h"


static ModPlug_Settings* modplugSettings = nullptr;


static void
initModPlug(const SDL_AudioSpec& spec)
{
    modplugSettings = new ModPlug_Settings;
    ModPlug_GetSettings(modplugSettings);
    modplugSettings->mFlags =   MODPLUG_ENABLE_OVERSAMPLING
                              | MODPLUG_ENABLE_NOISE_REDUCTION;
    // TODO: can modplug handle more than 2 channels?
    modplugSettings->mChannels = spec.channels == 1 ? 1 : 2;
    // It seems MogPlug does resample to any samplerate. 32, 44.1, up to
    // 192K all seem to work correctly.
    modplugSettings->mFrequency = spec.freq;
    modplugSettings->mResamplingMode = MODPLUG_RESAMPLE_FIR;
    modplugSettings->mBits = 32;
    ModPlug_SetSettings(modplugSettings);
}


namespace Aulib {

/// \private
class AudioDecoderModPlug_priv {
    friend class AudioDecoderModPlug;

    AudioDecoderModPlug_priv();
    ~AudioDecoderModPlug_priv();

    ModPlugFile* mpHandle;
    bool atEOF;
    float fDuration;
};

} // namespace Aulib


Aulib::AudioDecoderModPlug_priv::AudioDecoderModPlug_priv()
    : mpHandle(nullptr),
      atEOF(false),
      fDuration(-1.f)
{
    if (modplugSettings == nullptr) {
        initModPlug(Aulib::spec());
    }
}


Aulib::AudioDecoderModPlug_priv::~AudioDecoderModPlug_priv()
{
    if (mpHandle) {
        ModPlug_Unload(mpHandle);
    }
}


Aulib::AudioDecoderModPlug::AudioDecoderModPlug()
    : d(new AudioDecoderModPlug_priv)
{
}


Aulib::AudioDecoderModPlug::~AudioDecoderModPlug()
{
    delete d;
}


bool
Aulib::AudioDecoderModPlug::open(SDL_RWops* rwops)
{
    if (d->mpHandle) {
        return true;
    }
    int frontPos = SDL_RWtell(rwops);
    int dataSize = SDL_RWseek(rwops, 0, RW_SEEK_END) - frontPos;
    SDL_RWseek(rwops, frontPos, RW_SEEK_SET);
    if (dataSize <= 0) {
        return false;
    }
    bool ret = true;
    Uint8* data = new Uint8[dataSize];
    if (SDL_RWread(rwops, data, dataSize, 1) != 1) {
        ret = false;
    } else if ((d->mpHandle = ModPlug_Load(data, dataSize)) == nullptr) {
        ret = false;
    }
    ModPlug_SetMasterVolume(d->mpHandle, 192);
    d->fDuration = (float)ModPlug_GetLength(d->mpHandle) / 1000.f;
    delete[] data;
    return ret;
}


unsigned
Aulib::AudioDecoderModPlug::getChannels() const
{
    return modplugSettings->mChannels;
}


unsigned
Aulib::AudioDecoderModPlug::getRate() const
{
    return modplugSettings->mFrequency;
}


int
Aulib::AudioDecoderModPlug::doDecoding(float buf[], int len, bool& callAgain)
{
    callAgain = false;
    if (d->atEOF) {
        return 0;
    }
    Sint32* data = new Sint32[len];
    int ret = ModPlug_Read(d->mpHandle, data, len * 4);
    // Convert from 32-bit to float.
    for (int i = 0; i < len; ++i) {
        buf[i] = (float)data[i] / 2147483648.f;
    }
    if (ret == 0) {
        d->atEOF = true;
    }
    delete[] data;
    return ret / sizeof(*buf);
}


bool
Aulib::AudioDecoderModPlug::rewind()
{
    ModPlug_Seek(d->mpHandle, 0);
    d->atEOF = false;
    return true;
}


float
Aulib::AudioDecoderModPlug::duration() const
{
    return d->fDuration;
}


bool
Aulib::AudioDecoderModPlug::seekToTime(float seconds)
{
    ModPlug_Seek(d->mpHandle, seconds * 1000);
    return true;
}
