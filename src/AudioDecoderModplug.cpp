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

#include <limits>
#include <libmodplug/modplug.h>
#include <SDL_audio.h>

#include "aulib.h"
#include "Buffer.h"

static ModPlug_Settings modplugSettings;
static bool initialized = false;


static void
initModPlug(const SDL_AudioSpec& spec)
{
    ModPlug_GetSettings(&modplugSettings);
    modplugSettings.mFlags = MODPLUG_ENABLE_OVERSAMPLING | MODPLUG_ENABLE_NOISE_REDUCTION;
    // TODO: can modplug handle more than 2 channels?
    modplugSettings.mChannels = spec.channels == 1 ? 1 : 2;
    // It seems MogPlug does resample to any samplerate. 32, 44.1, up to
    // 192K all seem to work correctly.
    modplugSettings.mFrequency = spec.freq;
    modplugSettings.mResamplingMode = MODPLUG_RESAMPLE_FIR;
    modplugSettings.mBits = 32;
    ModPlug_SetSettings(&modplugSettings);
    initialized = true;
}


namespace Aulib {

/// \private
struct AudioDecoderModPlug_priv final {
    friend class AudioDecoderModPlug;

    AudioDecoderModPlug_priv();

    std::unique_ptr<ModPlugFile, decltype(&ModPlug_Unload)> mpHandle;
    bool atEOF;
    float fDuration;
};

} // namespace Aulib


Aulib::AudioDecoderModPlug_priv::AudioDecoderModPlug_priv()
    : mpHandle(nullptr, &ModPlug_Unload),
      atEOF(false),
      fDuration(-1.f)
{
    if (not initialized) {
        initModPlug(Aulib::spec());
    }
}


Aulib::AudioDecoderModPlug::AudioDecoderModPlug()
    : d(std::make_unique<AudioDecoderModPlug_priv>())
{
}


Aulib::AudioDecoderModPlug::~AudioDecoderModPlug()
{ }


bool
Aulib::AudioDecoderModPlug::open(SDL_RWops* rwops)
{
    if (isOpen()) {
        return true;
    }
    //FIXME: error reporting
    Sint64 dataSize = SDL_RWsize(rwops);
    if (dataSize <= 0 or dataSize > std::numeric_limits<int>::max()) {
        return false;
    }
    Buffer<Uint8> data((size_t)dataSize);
    if (SDL_RWread(rwops, data.get(), data.size(), 1) != 1) {
        return false;
    }
    d->mpHandle.reset(ModPlug_Load(data.get(), (int)data.size()));
    if (not d->mpHandle) {
        return false;
    }
    ModPlug_SetMasterVolume(d->mpHandle.get(), 192);
    d->fDuration = (float)ModPlug_GetLength(d->mpHandle.get()) / 1000.f;
    setIsOpen(true);
    return true;
}


unsigned
Aulib::AudioDecoderModPlug::getChannels() const
{
    return modplugSettings.mChannels;
}


unsigned
Aulib::AudioDecoderModPlug::getRate() const
{
    return modplugSettings.mFrequency;
}


size_t
Aulib::AudioDecoderModPlug::doDecoding(float buf[], size_t len, bool& callAgain)
{
    callAgain = false;
    if (d->atEOF) {
        return 0;
    }
    Buffer<Sint32> tmpBuf(len);
    int ret = ModPlug_Read(d->mpHandle.get(), tmpBuf.get(), len * 4);
    // Convert from 32-bit to float.
    for (size_t i = 0; i < len; ++i) {
        buf[i] = (float)tmpBuf[i] / 2147483648.f;
    }
    if (ret == 0) {
        d->atEOF = true;
    }
    return ret / sizeof(*buf);
}


bool
Aulib::AudioDecoderModPlug::rewind()
{
    ModPlug_Seek(d->mpHandle.get(), 0);
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
    ModPlug_Seek(d->mpHandle.get(), seconds * 1000);
    return true;
}
