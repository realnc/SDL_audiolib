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
#include "sdl_mixer_emu.h"
#include "aulib_global.h"
#include "aulib_config.h"
#include "aulib_debug.h"
#include "audiostream.h"
#include "resamp_speex.h"
#include "resamp_src.h"
#include "resamp_sox.h"
#include "dec_vorbis.h"
#include "dec_mpg123.h"
#include "dec_modplug.h"
#include "dec_fluidsynth.h"
#include "dec_sndfile.h"


// Currently active global music stream (SDL_mixer only supports one.)
static Aulib::AudioStream* gMusic = nullptr;
static float gMusicVolume = 1.0f;

extern "C" {
static void (*gMusicFinishHook)() = nullptr;
}

static void
musicFinishHookWrapper(Aulib::Stream&)
{
    if (gMusicFinishHook) {
        gMusicFinishHook();
    }
}


Mix_Music*
Mix_LoadMUS(const char* file)
{
    AM_debugPrintLn(__func__);

    Aulib::AudioDecoder* decoder = Aulib::AudioDecoder::decoderFor(file);
    Aulib::AudioStream* strm = new Aulib::AudioStream(file, decoder, new Aulib::AudioResamplerSpeex);
    strm->open();
    return (Mix_Music*)strm;
}


Mix_Music*
Mix_LoadMUS_RW(SDL_RWops* rw)
{
    AM_debugPrintLn(__func__);

    Aulib::AudioDecoder* decoder = Aulib::AudioDecoder::decoderFor(rw);
    Aulib::AudioStream* strm = new Aulib::AudioStream(rw, decoder, new Aulib::AudioResamplerSpeex);
    strm->open();
    return (Mix_Music*)strm;
}


Mix_Music*
Mix_LoadMUSType_RW(SDL_RWops* rw, Mix_MusicType type, int /*freesrc*/)
{
    AM_debugPrintLn(__func__ << " type: " << type);

    Aulib::AudioDecoder* decoder = nullptr;
    switch (type) {
        case MUS_WAV:
        case MUS_FLAC:
            decoder = new Aulib::AudioDecoderSndfile;
            break;

        case MUS_MOD:
        case MUS_MODPLUG:
            decoder = new Aulib::AudioDecoderModPlug;
            break;

        case MUS_MID:
            decoder = new Aulib::AudioDecoderFluidSynth;
            break;

        case MUS_OGG:
            decoder = new Aulib::AudioDecoderVorbis;
            break;

        case MUS_MP3:
        case MUS_MP3_MAD:
            decoder = new Aulib::AudioDecoderMpg123;
            break;

        default:
            AM_debugPrintLn("NO DECODER FOUND");
            return nullptr;
    }

    Aulib::AudioStream* strm = new Aulib::AudioStream(rw, decoder, new Aulib::AudioResamplerSpeex);
    strm->open();
    return (Mix_Music*)strm;
}


void
Mix_FreeMusic(Mix_Music* music)
{
    AM_debugPrintLn(__func__);

    Aulib::AudioStream* strm = (Aulib::AudioStream*)music;
    if (gMusic == strm) {
        gMusic = nullptr;
    }
    delete strm;
}


int
Mix_GetNumMusicDecoders()
{
    AM_debugPrintLn(__func__);

    return 0;
}


const char*
Mix_GetMusicDecoder(int /*index*/)
{
    AM_debugPrintLn(__func__);

    return nullptr;
}


Mix_MusicType
Mix_GetMusicType(const Mix_Music* /*music*/)
{
    AM_debugPrintLn(__func__);

    return MUS_NONE;
}


void
Mix_HookMusic(void (*/*mix_func*/)(void*, Uint8*, int), void* /*arg*/)
{
    AM_debugPrintLn(__func__);
}


void
Mix_HookMusicFinished(void (*music_finished)())
{
    AM_debugPrintLn(__func__);

    if (music_finished == nullptr) {
        if (gMusic) {
            gMusic->unsetFinishCallback();
        }
        gMusicFinishHook = nullptr;
    } else {
        gMusicFinishHook = music_finished;
        if (gMusic) {
            gMusic->setFinishCallback(musicFinishHookWrapper);
        }
    }
}


void*
Mix_GetMusicHookData()
{
    AM_debugPrintLn(__func__);

    return nullptr;
}


int
Mix_PlayMusic(Mix_Music* music, int loops)
{
    AM_debugPrintLn(__func__);

    if (gMusic) {
        gMusic->stop();
        gMusic = nullptr;
    }
    if (loops == 0) {
        return 0;
    }
    gMusic = (Aulib::AudioStream*)music;
    gMusic->setVolume(gMusicVolume);
    return gMusic->play(loops == -1 ? 0 : loops);
}


int
Mix_FadeInMusic(Mix_Music* music, int loops, int /*ms*/)
{
    AM_debugPrintLn(__func__);

    return Mix_PlayMusic(music, loops);
}


int
Mix_FadeInMusicPos(Mix_Music* /*music*/, int /*loops*/, int /*ms*/, double /*position*/)
{
    AM_debugPrintLn(__func__);

    return 0;
}


int
Mix_VolumeMusic(int volume)
{
    AM_debugPrintLn(__func__);

    int prevVol = gMusicVolume * MIX_MAX_VOLUME;
    volume = std::min(std::max(-1, volume), MIX_MAX_VOLUME);

    if (volume >= 0) {
        gMusicVolume = (float)volume / (float)MIX_MAX_VOLUME;
        if (gMusic) {
            gMusic->setVolume(gMusicVolume);
        }
    }
    return prevVol;
}


int
Mix_HaltMusic()
{
    AM_debugPrintLn(__func__);

    if (gMusic) {
        gMusic->stop();
        gMusic = nullptr;
    }
    return 0;
}


int
Mix_FadeOutMusic(int /*ms*/)
{
    AM_debugPrintLn(__func__);

    return 0;
}


Mix_Fading
Mix_FadingMusic()
{
    AM_debugPrintLn(__func__);

    return MIX_NO_FADING;
}


void
Mix_PauseMusic()
{
    AM_debugPrintLn(__func__);

    if (gMusic) {
        gMusic->pause();
    }
}


void
Mix_ResumeMusic()
{
    AM_debugPrintLn(__func__);

    if (gMusic and gMusic->isPaused()) {
        gMusic->resume();
    }
}


void
Mix_RewindMusic()
{
    AM_debugPrintLn(__func__);

    if (gMusic) {
        gMusic->rewind();
    }
}


int
Mix_PausedMusic()
{
    AM_debugPrintLn(__func__);

    if (gMusic) {
        return gMusic->isPaused();
    }
    return false;
}


int
Mix_SetMusicPosition(double /*position*/)
{
    AM_debugPrintLn(__func__);

    return -1;
}


int
Mix_PlayingMusic()
{
    AM_debugPrintLn(__func__);

    if (gMusic) {
        return gMusic->isPlaying();
    }
    return false;
}


int
Mix_SetMusicCMD(const char* /*command*/)
{
    AM_debugPrintLn(__func__);

    return -1;
}


int
Mix_SetSynchroValue(int /*value*/)
{
    AM_debugPrintLn(__func__);

    return -1;
}


int
Mix_GetSynchroValue()
{
    AM_debugPrintLn(__func__);

    return -1;
}


int
Mix_SetSoundFonts(const char* /*paths*/)
{
    AM_debugPrintLn(__func__);

    return 0;
}


const char*
Mix_GetSoundFonts()
{
    AM_debugPrintLn(__func__);

    return nullptr;
}


int
Mix_EachSoundFont(int (*/*function*/)(const char*, void*), void* /*data*/)
{
    AM_debugPrintLn(__func__);

    return 0;
}
