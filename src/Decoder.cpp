// This is copyrighted software. More information is at the end of this file.
#include "Aulib/Decoder.h"

#include "Aulib/DecoderAdlmidi.h"
#include "Aulib/DecoderBassmidi.h"
#include "Aulib/DecoderDrflac.h"
#include "Aulib/DecoderDrmp3.h"
#include "Aulib/DecoderDrwav.h"
#include "Aulib/DecoderFluidsynth.h"
#include "Aulib/DecoderModplug.h"
#include "Aulib/DecoderMpg123.h"
#include "Aulib/DecoderMusepack.h"
#include "Aulib/DecoderOpenmpt.h"
#include "Aulib/DecoderOpus.h"
#include "Aulib/DecoderSndfile.h"
#include "Aulib/DecoderVorbis.h"
#include "Aulib/DecoderWildmidi.h"
#include "Aulib/DecoderXmp.h"
#include "Buffer.h"
#include "aulib.h"
#include "aulib_config.h"
#include "missing.h"
#include <SDL_audio.h>
#include <SDL_rwops.h>
#include <array>

namespace Aulib {

template <typename T>
struct Decoder_priv final
{
    Buffer<T> stereoBuf{0};
    bool isOpen = false;
};

} // namespace Aulib

template <typename T>
Aulib::Decoder<T>::Decoder()
    : d(std::make_unique<Aulib::Decoder_priv<T>>())
{}

template <typename T>
Aulib::Decoder<T>::~Decoder() = default;

template <typename T>
auto Aulib::Decoder<T>::decoderFor(const std::string& filename) -> std::unique_ptr<Decoder<T>>
{
    auto rwopsClose = [](SDL_RWops* rwops) { SDL_RWclose(rwops); };
    std::unique_ptr<SDL_RWops, decltype(rwopsClose)> rwops(
        SDL_RWFromFile(filename.c_str(), "rb"), rwopsClose);
    return Decoder<T>::decoderFor(rwops.get());
}

namespace {

class DecoderBuilder
{
public:
    explicit DecoderBuilder(SDL_RWops* rwops)
        : fRwops(rwops)
        , fRwPos(SDL_RWtell(rwops))
    { }

    template <typename DecoderT>
    auto build() -> std::unique_ptr<DecoderT>;

private:
    void rewind()
    {
        SDL_RWseek(fRwops, fRwPos, RW_SEEK_SET);
    }

    SDL_RWops* fRwops;
    decltype(SDL_RWtell(nullptr)) fRwPos;
};

template <typename DecoderT>
auto DecoderBuilder::build() -> std::unique_ptr<DecoderT>
{
    auto decoder = std::make_unique<DecoderT>();
    rewind();
    const bool ok = decoder->open(fRwops);
    rewind();
    if (!ok)
        return nullptr;
    return decoder;
}

// `int32_t` not supported
template <>
auto DecoderBuilder::build<Aulib::DecoderOpenmpt<int32_t>>()
    -> std::unique_ptr<Aulib::DecoderOpenmpt<int32_t>>
{
    return nullptr;
}

// `int32_t` not supported
template <>
auto DecoderBuilder::build<Aulib::DecoderDrmp3<int32_t>>()
    -> std::unique_ptr<Aulib::DecoderDrmp3<int32_t>>
{
    return nullptr;
}

} // namespace

template <typename T>
auto Aulib::Decoder<T>::decoderFor(SDL_RWops* rwops) -> std::unique_ptr<Aulib::Decoder<T>>
{
    DecoderBuilder builder{rwops};
    std::unique_ptr<Decoder<T>> result;

#if USE_DEC_DRFLAC
    if ((result = builder.build<DecoderDrflac<T>>()) != nullptr) {
        return result;
    }
#endif
#if USE_DEC_LIBVORBIS
    if ((result = builder.build<DecoderVorbis<T>>()) != nullptr) {
        return result;
    }
#endif
#if USE_DEC_LIBOPUSFILE
    if ((result = builder.build<DecoderOpus<T>>()) != nullptr) {
        return result;
    }
#endif
#if USE_DEC_MUSEPACK
    if ((result = builder.build<DecoderMusepack<T>>()) != nullptr) {
        return result;
    }
#endif
#if USE_DEC_FLUIDSYNTH or USE_DEC_BASSMIDI or USE_DEC_WILDMIDI or USE_DEC_ADLMIDI
    {
        std::array<char, 5> head{};
        if (SDL_RWread(rwops, head.data(), 1, 4) == 4 and head == decltype(head){"MThd"}) {
            using midi_dec_type =
#if USE_DEC_FLUIDSYNTH
                DecoderFluidsynth<T>;
#elif USE_DEC_BASSMIDI
                DecoderBassmidi<T>;
#elif USE_DEC_WILDMIDI
                DecoderWildmidi<T>;
#elif USE_DEC_ADLMIDI
                DecoderAdlmidi<T>;
#endif
            if ((result = builder.build<midi_dec_type>()) != nullptr) {
                return result;
            }
        }
    }
#endif
#if USE_DEC_SNDFILE
    if ((result = builder.build<DecoderSndfile<T>>()) != nullptr) {
        return result;
    }
#endif
#if USE_DEC_DRWAV
    if ((result = builder.build<DecoderDrwav<T>>()) != nullptr) {
        return result;
    }
#endif
#if USE_DEC_OPENMPT
    if ((result = builder.build<DecoderOpenmpt<T>>()) != nullptr) {
        return result;
    }
#endif

#if USE_DEC_XMP
    if ((result = builder.build<DecoderXmp<T>>()) != nullptr) {
        return result;
    }
#endif
#if USE_DEC_MODPLUG
    // We don't try ModPlug, since it thinks just about anything is a module
    // file, which would result in virtually everything we feed it giving a
    // false positive.
#endif

// The MP3 decoders have too many false positives. So try them last.
#if USE_DEC_MPG123
    if ((result = builder.build<DecoderMpg123<T>>()) != nullptr) {
        return result;
    }
#endif
#if USE_DEC_DRMP3
    if ((result = builder.build<DecoderDrmp3<T>>()) != nullptr) {
        return result;
    }
#endif
    return nullptr;
}

template <typename T>
auto Aulib::Decoder<T>::isOpen() const -> bool
{
    return d->isOpen;
}

// Conversion happens in-place.
template <typename T>
static constexpr void monoToStereo(T buf[], int len)
{
    if (len < 1 or not buf) {
        return;
    }
    for (int i = len / 2 - 1, j = len - 1; i >= 0; --i) {
        buf[j--] = buf[i];
        buf[j--] = buf[i];
    }
}

template <typename T>
static constexpr void stereoToMono(T dst[], const T src[], int srcLen)
{
    if (srcLen < 1 or not dst or not src) {
        return;
    }
    for (int i = 0, j = 0; i < srcLen; i += 2, ++j) {
        dst[j] = src[i] / 2;
        dst[j] += src[i + 1] / 2;
    }
}

template <typename T>
auto Aulib::Decoder<T>::decode(T buf[], int len, bool& callAgain) -> int
{
    if (this->getChannels() == 1 and Aulib::channelCount() == 2) {
        int srcLen = this->doDecoding(buf, len / 2, callAgain);
        monoToStereo(buf, srcLen * 2);
        return srcLen * 2;
    }

    if (this->getChannels() == 2 and Aulib::channelCount() == 1) {
        if (d->stereoBuf.size() != len * 2) {
            d->stereoBuf.reset(len * 2);
        }
        int srcLen = this->doDecoding(d->stereoBuf.get(), d->stereoBuf.size(), callAgain);
        stereoToMono(buf, d->stereoBuf.get(), srcLen);
        return srcLen / 2;
    }
    return this->doDecoding(buf, len, callAgain);
}

template <typename T>
void Aulib::Decoder<T>::setIsOpen(bool f)
{
    d->isOpen = f;
}

template class Aulib::Decoder<float>;
template class Aulib::Decoder<int32_t>;

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
