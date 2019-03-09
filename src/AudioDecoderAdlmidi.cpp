// This is copyrighted software. More information is at the end of this file.
#include "Aulib/AudioDecoderAdlmidi.h"

#include "Buffer.h"
#include "aulib.h"
#include <SDL_rwops.h>
#include <adlmidi.h>

namespace chrono = std::chrono;

static constexpr int SAMPLE_RATE = 49716;

namespace Aulib {

/// \private
struct AudioDecoderAdlmidi_priv final
{
    std::unique_ptr<ADL_MIDIPlayer, decltype(&adl_close)> adl_player{nullptr, adl_close};
    Buffer<Uint8> midi_data{0};
    bool eof = false;
    chrono::microseconds duration{};
};

} // namespace Aulib

Aulib::AudioDecoderAdlmidi::AudioDecoderAdlmidi()
    : d(std::make_unique<AudioDecoderAdlmidi_priv>())
{}

Aulib::AudioDecoderAdlmidi::~AudioDecoderAdlmidi() = default;

bool Aulib::AudioDecoderAdlmidi::open(SDL_RWops* rwops)
{
    if (isOpen()) {
        return true;
    }
    Sint64 midiDataLen = SDL_RWsize(rwops);
    if (midiDataLen <= 0) {
        SDL_SetError("Tried to open zero-length MIDI data source.");
        return false;
    }
    Buffer<Uint8> new_midi_data(midiDataLen);
    if (SDL_RWread(rwops, new_midi_data.get(), new_midi_data.size(), 1) != 1) {
        SDL_SetError("Failed to read MIDI data.");
        return false;
    }
    d->adl_player.reset(adl_init(SAMPLE_RATE));
    if (d->adl_player == nullptr) {
        SDL_SetError("Failed to initialize libADLMIDI: %s", adl_errorString());
        return false;
    }
    // TODO: Add API for setting these?
    adl_setBank(d->adl_player.get(), 65);
    adl_setNumChips(d->adl_player.get(), 4);
    if (adl_openData(d->adl_player.get(), new_midi_data.get(), new_midi_data.size()) != 0) {
        SDL_SetError("libADLMIDI failed to open MIDI data: %s", adl_errorInfo(d->adl_player.get()));
        return false;
    }
    d->duration = chrono::duration_cast<chrono::microseconds>(
        chrono::duration<double>(adl_totalTimeLength(d->adl_player.get())));
    d->midi_data.swap(new_midi_data);
    setIsOpen(true);
    return true;
}

int Aulib::AudioDecoderAdlmidi::getChannels() const
{
    return 2;
}

int Aulib::AudioDecoderAdlmidi::getRate() const
{
    return SAMPLE_RATE;
}

bool Aulib::AudioDecoderAdlmidi::rewind()
{
    if (d->adl_player == nullptr) {
        return false;
    }
    adl_positionRewind(d->adl_player.get());
    d->eof = false;
    return true;
}

chrono::microseconds Aulib::AudioDecoderAdlmidi::duration() const
{
    return d->duration;
}

bool Aulib::AudioDecoderAdlmidi::seekToTime(chrono::microseconds pos)
{
    if (d->adl_player == nullptr) {
        return false;
    }
    adl_positionSeek(d->adl_player.get(), chrono::duration<double>(pos).count());
    d->eof = false;
    return true;
}

int Aulib::AudioDecoderAdlmidi::doDecoding(float buf[], int len, bool& callAgain)
{
    callAgain = false;
    if (d->adl_player == nullptr or d->eof) {
        return 0;
    }
    constexpr ADLMIDI_AudioFormat adl_format{ADLMIDI_SampleType_F32, sizeof(float),
                                             sizeof(float) * 2};
    int sample_count = adl_playFormat(d->adl_player.get(), len, (ADL_UInt8*)buf,
                                      (ADL_UInt8*)(buf + 1), &adl_format);
    if (sample_count < len) {
        d->eof = true;
        return 0;
    }
    return sample_count;
}

/*

Copyright (C) 2014, 2015, 2016, 2017, 2018 Nikos Chantziaras.

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
