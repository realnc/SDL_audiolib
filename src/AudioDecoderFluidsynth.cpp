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
#include "Aulib/AudioDecoderFluidsynth.h"

#include <SDL_rwops.h>
#include <SDL_audio.h>
#include <fluidsynth.h>

#include "aulib.h"
#include "aulib_debug.h"
#include "Buffer.h"


static fluid_settings_t* settings = nullptr;


static int
initFluidSynth()
{
    if (settings) {
        return 0;
    }
    if ((settings = new_fluid_settings()) == nullptr) {
        return -1;
    }
    fluid_settings_setnum(settings, "synth.sample-rate", Aulib::spec().freq);
    return 0;
}


namespace Aulib {

/// \private
struct AudioDecoderFluidSynth_priv final {
    friend class AudioDecoderFluidSynth;

    AudioDecoderFluidSynth_priv();

    std::unique_ptr<fluid_synth_t, decltype(&delete_fluid_synth)> fSynth;
    std::unique_ptr<fluid_player_t, decltype(&delete_fluid_player)> fPlayer;
    Buffer<Uint8> fMidiData;
    bool fEOF;
};

} // namespace Aulib


Aulib::AudioDecoderFluidSynth_priv::AudioDecoderFluidSynth_priv()
    : fSynth(nullptr, &delete_fluid_synth),
      fPlayer(nullptr, &delete_fluid_player),
      fMidiData(0),
      fEOF(false)
{
    if (settings == nullptr) {
        initFluidSynth();
    }
    fSynth.reset(new_fluid_synth(settings));
    if (not fSynth) {
        return;
    }
    fluid_synth_set_gain(fSynth.get(), 0.6f);
    fluid_synth_set_interp_method(fSynth.get(), -1, FLUID_INTERP_7THORDER);
    fluid_synth_set_reverb(fSynth.get(),
                           0.6,  // Room size
                           0.5,  // Damping
                           0.5,  // Width
                           0.3); // Level
}


Aulib::AudioDecoderFluidSynth::AudioDecoderFluidSynth()
    : d(std::make_unique<AudioDecoderFluidSynth_priv>())
{
}


Aulib::AudioDecoderFluidSynth::~AudioDecoderFluidSynth()
{ }


int
Aulib::AudioDecoderFluidSynth::loadSoundfont(const char filename[])
{
    fluid_synth_sfload(d->fSynth.get(), filename, true);
    return 0;
}


bool
Aulib::AudioDecoderFluidSynth::open(SDL_RWops* rwops)
{
    if (isOpen()) {
        return true;
    }
    if (not d->fSynth) {
        return false;
    }

    //FIXME: error reporting
    Sint64 midiDataLen = SDL_RWsize(rwops);
    if (midiDataLen <= 0) {
        return false;
    }
    Buffer<Uint8> newMidiData((size_t)midiDataLen);
    if (SDL_RWread(rwops, newMidiData.get(), newMidiData.size(), 1) != 1) {
        return false;
    }
    d->fPlayer.reset(new_fluid_player(d->fSynth.get()));
    if (not d->fPlayer
        or fluid_player_add_mem(d->fPlayer.get(), newMidiData.get(), newMidiData.size()) != FLUID_OK
        or fluid_player_play(d->fPlayer.get()) != FLUID_OK)
    {
        return false;
    }
    d->fMidiData.swap(newMidiData);
    setIsOpen(true);
    return true;
}


unsigned
Aulib::AudioDecoderFluidSynth::getChannels() const
{
    int channels;
    fluid_settings_getint(settings, "synth.audio-channels", &channels);
    // This is the amount of stereo channel *pairs*, so each pair has *two* audio channels.
    return channels * 2;
}

unsigned
Aulib::AudioDecoderFluidSynth::getRate() const
{
    double rate;
    fluid_settings_getnum(settings, "synth.sample-rate", &rate);
    return rate;
}


size_t
Aulib::AudioDecoderFluidSynth::doDecoding(float buf[], size_t len, bool& callAgain)
{
    callAgain = false;
    if (not d->fPlayer or d->fEOF) {
        return 0;
    }

    len /= Aulib::spec().channels;
    int res = fluid_synth_write_float(d->fSynth.get(), len, buf, 0, 2, buf, 1, 2);
    if (fluid_player_get_status(d->fPlayer.get()) == FLUID_PLAYER_DONE) {
        d->fEOF = true;
    }
    if (res == FLUID_OK) {
        return len * Aulib::spec().channels;
    }
    return 0;
}


bool
Aulib::AudioDecoderFluidSynth::rewind()
{
    fluid_player_stop(d->fPlayer.get());
    d->fPlayer.reset(new_fluid_player(d->fSynth.get()));
    if (not d->fPlayer) {
        return false;
    }
    fluid_player_add_mem(d->fPlayer.get(), d->fMidiData.get(), d->fMidiData.size());
    fluid_player_play(d->fPlayer.get());
    d->fEOF = false;
    return true;
}


float
Aulib::AudioDecoderFluidSynth::duration() const
{
    // We can't tell how long a MIDI file is.
    return -1.f;
}


bool
Aulib::AudioDecoderFluidSynth::seekToTime(float)
{
    // We don't support seeking.
    return false;
}
