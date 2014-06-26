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
class AudioDecoderFluidSynth_priv {
    friend class AudioDecoderFluidSynth;

    AudioDecoderFluidSynth_priv();
    ~AudioDecoderFluidSynth_priv();

    fluid_synth_t* fSynth;
    fluid_player_t* fPlayer;
    Uint8* fMidiData;
    int fMidiDataLen;
    bool fEOF;
};

} // namespace Aulib


Aulib::AudioDecoderFluidSynth_priv::AudioDecoderFluidSynth_priv()
    : fPlayer(nullptr),
      fMidiData(nullptr),
      fMidiDataLen(0),
      fEOF(false)
{
    if (settings == nullptr) {
        initFluidSynth();
    }
    if ((fSynth = new_fluid_synth(settings)) == nullptr) {
        return;
    }
    fluid_synth_set_gain(fSynth, 0.7f);
    fluid_synth_set_interp_method(fSynth, -1, FLUID_INTERP_7THORDER);
}


Aulib::AudioDecoderFluidSynth_priv::~AudioDecoderFluidSynth_priv()
{
    delete_fluid_player(fPlayer);
    delete_fluid_synth(fSynth);
    delete[] fMidiData;
}


Aulib::AudioDecoderFluidSynth::AudioDecoderFluidSynth()
    : d(new AudioDecoderFluidSynth_priv)
{
}


Aulib::AudioDecoderFluidSynth::~AudioDecoderFluidSynth()
{
    delete d;
}


int
Aulib::AudioDecoderFluidSynth::loadSoundfont(const char filename[])
{
    fluid_synth_sfload(d->fSynth, filename, true);
    return 0;
}


bool
Aulib::AudioDecoderFluidSynth::open(SDL_RWops* rwops)
{
    if (d->fSynth == nullptr) {
        return false;
    }

    int frontPos = SDL_RWtell(rwops);
    d->fMidiDataLen = SDL_RWseek(rwops, 0, RW_SEEK_END) - frontPos;
    SDL_RWseek(rwops, frontPos, RW_SEEK_SET);
    if (d->fMidiDataLen <= 0) {
        return false;
    }

    d->fMidiData = new Uint8[d->fMidiDataLen];
    if (SDL_RWread(rwops, d->fMidiData, d->fMidiDataLen, 1) != 1
        or (d->fPlayer = new_fluid_player(d->fSynth)) == nullptr
        or fluid_player_add_mem(d->fPlayer, d->fMidiData, d->fMidiDataLen) != FLUID_OK
        or fluid_player_play(d->fPlayer) != FLUID_OK)
    {
        if (d->fPlayer) {
            delete_fluid_player(d->fPlayer);
        }
        if (d->fSynth) {
            delete_fluid_synth(d->fSynth);
            d->fSynth = nullptr;
        }
        delete[] d->fMidiData;
        return false;
    }
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


int
Aulib::AudioDecoderFluidSynth::doDecoding(float buf[], int len, bool& callAgain)
{
    callAgain = false;
    if (d->fSynth == nullptr or d->fEOF) {
        return 0;
    }

    len /= Aulib::spec().channels;
    int res = fluid_synth_write_float(d->fSynth, len, buf, 0, 2, buf, 1, 2);
    if (fluid_player_get_status(d->fPlayer) == FLUID_PLAYER_DONE) {
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
    fluid_player_stop(d->fPlayer);
    delete_fluid_player(d->fPlayer);
    d->fPlayer = new_fluid_player(d->fSynth);
    fluid_player_add_mem(d->fPlayer, d->fMidiData, d->fMidiDataLen);
    fluid_player_play(d->fPlayer);
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
