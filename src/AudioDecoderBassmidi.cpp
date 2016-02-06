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
#include "Aulib/AudioDecoderBassmidi.h"

#include <SDL_rwops.h>
#include <SDL_audio.h>
#include <bass.h>
#include <bassmidi.h>
#include "aulib.h"
#include "aulib_debug.h"
#include "Buffer.h"

static bool bassIsInitialized = false;


namespace Aulib {

/// \private
class AudioDecoderBassmidi_priv {
    friend class AudioDecoderBassmidi;

    AudioDecoderBassmidi_priv();
    ~AudioDecoderBassmidi_priv();

    HSTREAM hstream;
    Buffer<Uint8> midiData;
    bool eof;
};

} // namespace Aulib


Aulib::AudioDecoderBassmidi_priv::AudioDecoderBassmidi_priv()
    : hstream(0),
      midiData(0),
      eof(false)
{
    if (bassIsInitialized) {
        return;
    }
    if (BASS_Init(0, Aulib::spec().freq, 0, nullptr, nullptr)) {
        bassIsInitialized = true;
        return;
    }
    AM_debugPrintLn("AudioDecoderBassmidi: got BASS error " << BASS_ErrorGetCode() << " while initializing.");
}


Aulib::AudioDecoderBassmidi_priv::~AudioDecoderBassmidi_priv()
{
    if (hstream) {
        if (not BASS_StreamFree(hstream)) {
            AM_debugPrintLn("AudioDecoderBassmidi: got BASS error " << BASS_ErrorGetCode()
                            << " while freeing HSTREAM.");
        }
    }
}


Aulib::AudioDecoderBassmidi::AudioDecoderBassmidi()
    : d(new AudioDecoderBassmidi_priv)
{ }


Aulib::AudioDecoderBassmidi::~AudioDecoderBassmidi()
{
    delete d;
}


bool
Aulib::AudioDecoderBassmidi::setDefaultSoundfont(const char filename[])
{
    if (BASS_SetConfigPtr(BASS_CONFIG_MIDI_DEFFONT, filename)) {
        return true;
    }
    AM_debugPrintLn("AudioDecoderBassmidi: got BASS error " << BASS_ErrorGetCode()
                    << " while setting default soundfont.");
    return false;
}


bool
Aulib::AudioDecoderBassmidi::open(SDL_RWops* rwops)
{
    if (isOpen()) {
        return true;
    }

    Sint64 frontPos = SDL_RWtell(rwops);
    //FIXME: check for seek error
    Sint64 midiDataLen = SDL_RWseek(rwops, 0, RW_SEEK_END) - frontPos;
    if (midiDataLen == 0) {
        return false;
    }
    SDL_RWseek(rwops, frontPos, RW_SEEK_SET);
    Buffer<Uint8> newMidiData(midiDataLen);
    DWORD bassFlags = BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | BASS_MIDI_DECAYEND | BASS_MIDI_SINCINTER;
    if (SDL_RWread(rwops, newMidiData.get(), midiDataLen, 1) != 1
        or (d->hstream = BASS_MIDI_StreamCreateFile(TRUE, newMidiData.get(), 0, midiDataLen, bassFlags,
                                                    1)) == 0)
    {
        if (d->hstream) {
            BASS_StreamFree(d->hstream);
            d->hstream = 0;
        } else {
            AM_debugPrintLn("AudioDecoderBassmidi: got BASS error " << BASS_ErrorGetCode()
                            << " while creating HSTREAM.");
        }
        return false;
    }
    d->midiData.swap(newMidiData);
    setIsOpen(true);
    return true;
}


unsigned
Aulib::AudioDecoderBassmidi::getChannels() const
{
    return 2;
}


unsigned
Aulib::AudioDecoderBassmidi::getRate() const
{
    BASS_CHANNELINFO inf;
    if (BASS_ChannelGetInfo(d->hstream, &inf)) {
        return inf.freq;
    }
    AM_debugPrintLn("AudioDecoderBassmidi: got BASS error " << BASS_ErrorGetCode()
                    << " while getting BASS_CHANNELINFO");
    return 0;
}


size_t
Aulib::AudioDecoderBassmidi::doDecoding(float buf[], size_t len, bool&)
{
    if (d->eof or d->hstream == 0) {
        return 0;
    }

    DWORD byteLen = len * sizeof(*buf);
    DWORD ret = BASS_ChannelGetData(d->hstream, buf, byteLen | BASS_DATA_FLOAT);
    if (ret == (DWORD)-1) {
        SDL_SetError("AudioDecoderBassmidi: got BASS error %d during decoding.\n", BASS_ErrorGetCode());
        return 0;
    } else if (ret < byteLen) {
        d->eof = true;
    }
    return ret / sizeof(*buf);
}


bool
Aulib::AudioDecoderBassmidi::rewind()
{
    if (d->hstream == 0) {
        return false;
    }

    if (BASS_ChannelSetPosition(d->hstream, 0, BASS_POS_BYTE)) {
        d->eof = false;
        return true;
    }
    SDL_SetError("AudioDecoderBassmidi: got BASS error %d during rewinding.\n", BASS_ErrorGetCode());
    return false;
}


float
Aulib::AudioDecoderBassmidi::duration() const
{
    if (d->hstream == 0) {
        return -1;
    }

    QWORD pos = BASS_ChannelGetLength(d->hstream, BASS_POS_BYTE);
    if (pos == (QWORD)-1) {
        AM_debugPrintLn("AudioDecoderBassmidi: got BASS error " << BASS_ErrorGetCode()
                        << " while getting channel length.");
        return -1;
    }
    double sec = BASS_ChannelBytes2Seconds(d->hstream, pos);
    if (sec < 0) {
        AM_debugPrintLn("AudioDecoderBassmidi: got BASS error " << BASS_ErrorGetCode()
                        << " while translating duration from bytes to seconds.");
        return -1;
    }
    return sec;
}


bool
Aulib::AudioDecoderBassmidi::seekToTime(float seconds)
{
    if (d->hstream == 0) {
        return false;
    }

    QWORD bytePos = BASS_ChannelSeconds2Bytes(d->hstream, seconds);
    if (bytePos == (QWORD)-1) {
        AM_debugPrintLn("AudioDecoderBassmidi: got BASS error " << BASS_ErrorGetCode()
                        << " while translating seek time to bytes.");
        return false;
    }
    if (BASS_ChannelSetPosition(d->hstream, bytePos, BASS_POS_BYTE)) {
        return true;
    }
    AM_debugPrintLn("AudioDecoderBassmidi: got BASS error " << BASS_ErrorGetCode()
                    << " while trying to seek.");
    return false;
}
