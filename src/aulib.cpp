// This is copyrighted software. More information is at the end of this file.
#include "aulib.h"

#include "Aulib/Stream.h"
#include "aulib_log.h"
#include "missing.h"
#include "sampleconv.h"
#include "stream_p.h"
#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_version.h>

enum class InitType
{
    None,
    NoOutput,
    Full,
};

static InitType gInitType = InitType::None;

extern "C" {
static void sdlCallback(void* /*unused*/, Uint8 out[], int outLen)
{
    switch (Aulib::Stream_priv_device::fDataType) {
    case Aulib::BufferDataType::FLOAT:
        Aulib::Stream_priv<float>::fSdlCallbackImpl(nullptr, out, outLen);
        break;
    case Aulib::BufferDataType::INT32:
        Aulib::Stream_priv<int32_t>::fSdlCallbackImpl(nullptr, out, outLen);
        break;
    }
}
}

namespace {

auto getFloatSampleConverter(uint16_t format) -> void (*)(Uint8[], const Buffer<float>& src)
{
    switch (format) {
    case AUDIO_S8:
        aulib::log::debugLn("S8");
        return Aulib::floatToS8;
        break;
    case AUDIO_U8:
        aulib::log::debugLn("U8");
        return Aulib::floatToU8;
    case AUDIO_S16LSB:
        aulib::log::debugLn("S16LSB");
        return Aulib::floatToS16LSB;
    case AUDIO_U16LSB:
        aulib::log::debugLn("U16LSB");
        return Aulib::floatToU16LSB;
    case AUDIO_S16MSB:
        aulib::log::debugLn("S16MSB");
        return Aulib::floatToS16MSB;
    case AUDIO_U16MSB:
        aulib::log::debugLn("U16MSB");
        return Aulib::floatToU16MSB;
#if SDL_VERSION_ATLEAST(2, 0, 0)
    case AUDIO_S32LSB:
        aulib::log::debugLn("S32LSB");
        return Aulib::floatToS32LSB;
    case AUDIO_S32MSB:
        aulib::log::debugLn("S32MSB");
        return Aulib::floatToS32MSB;
    case AUDIO_F32LSB:
        aulib::log::debugLn("F32LSB");
        return Aulib::floatToFloatLSB;
    case AUDIO_F32MSB:
        aulib::log::debugLn("F32MSB");
        return Aulib::floatToFloatMSB;
#endif
    default:
        return nullptr;
    }
}

auto getInt32SampleConverter(uint16_t format) -> void (*)(Uint8[], const Buffer<int32_t>& src)
{
    switch (format) {
    case AUDIO_S8:
        aulib::log::debugLn("S8");
        return Aulib::int32ToS8;
        break;
    case AUDIO_U8:
        aulib::log::debugLn("U8");
        return Aulib::int32ToU8;
    case AUDIO_S16LSB:
        aulib::log::debugLn("S16LSB");
        return Aulib::int32ToS16LSB;
    case AUDIO_U16LSB:
        aulib::log::debugLn("U16LSB");
        return Aulib::int32ToU16LSB;
    case AUDIO_S16MSB:
        aulib::log::debugLn("S16MSB");
        return Aulib::int32ToS16MSB;
    case AUDIO_U16MSB:
        aulib::log::debugLn("U16MSB");
        return Aulib::int32ToU16MSB;
#if SDL_VERSION_ATLEAST(2, 0, 0)
    case AUDIO_S32LSB:
        aulib::log::debugLn("S32LSB");
        return Aulib::int32ToS32LSB;
    case AUDIO_S32MSB:
        aulib::log::debugLn("S32MSB");
        return Aulib::int32ToS32MSB;
    case AUDIO_F32LSB:
        aulib::log::debugLn("F32LSB");
        return Aulib::int32ToFloatLSB;
    case AUDIO_F32MSB:
        aulib::log::debugLn("F32MSB");
        return Aulib::int32ToFloatMSB;
#endif
    default:
        return nullptr;
    }
}

} // namespace

auto Aulib::init(int freq, AudioFormat format, int channels, int frameSize, BufferDataType dataType)
    -> bool
{
    return init(freq, format, channels, frameSize, {}, dataType);
}

auto Aulib::init(
    int freq, AudioFormat format, int channels, int frameSize, const std::string& device,
    BufferDataType dataType) -> bool
{
    if (gInitType != InitType::None) {
        SDL_SetError("SDL_audiolib already initialized, cannot initialize again.");
        return false;
    }

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        return false;
    }

    // We only support mono and stereo at this point.
    channels = std::min(std::max(1, channels), 2);

    SDL_AudioSpec requestedSpec{};
    requestedSpec.freq = freq;
    requestedSpec.format = format;
    requestedSpec.channels = channels;
    requestedSpec.samples = frameSize;
    requestedSpec.callback = ::sdlCallback;
    Stream_priv_device::fAudioSpec = requestedSpec;
    Stream_priv_device::fDataType = dataType;
#if SDL_VERSION_ATLEAST(2, 0, 0)
    auto flags = SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_FORMAT_CHANGE;
#    if SDL_VERSION_ATLEAST(2, 0, 9)
    flags |= SDL_AUDIO_ALLOW_SAMPLES_CHANGE;
#    endif
    Stream_priv_device::fDeviceId = SDL_OpenAudioDevice(
        device.empty() ? nullptr : device.c_str(), false, &requestedSpec,
        &Stream_priv_device::fAudioSpec, flags);
    if (Stream_priv_device::fDeviceId == 0) {
        Aulib::quit();
        return false;
    }
#else
    if (SDL_OpenAudio(&requestedSpec, &Stream_priv_device::fAudioSpec) == -1) {
        Aulib::quit();
        return false;
    }
#endif

    aulib::log::debug("SDL initialized with sample format: ");
    bool sampleConverterFound = false;
    switch (dataType) {
    case BufferDataType::FLOAT:
        Stream_priv<float>::fSampleConverter =
            getFloatSampleConverter(Stream_priv_device::fAudioSpec.format);
        sampleConverterFound = Stream_priv<float>::fSampleConverter != nullptr;
        break;
    case BufferDataType::INT32:
        Stream_priv<int32_t>::fSampleConverter =
            getInt32SampleConverter(Stream_priv_device::fAudioSpec.format);
        sampleConverterFound = Stream_priv<int32_t>::fSampleConverter != nullptr;
        break;
    }
    if (!sampleConverterFound) {
        aulib::log::warnLn("Unknown audio format spec: {}", Stream_priv_device::fAudioSpec.format);
        Aulib::quit();
        return false;
    }

#if SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_PauseAudioDevice(Stream_priv_device::fDeviceId, false);
#else
    SDL_PauseAudio(/*pause_on=*/0);
#endif
    gInitType = InitType::Full;
    std::atexit(Aulib::quit);
    return true;
}

auto Aulib::initWithoutOutput(const int freq, const int channels) -> bool
{
    if (gInitType != InitType::None) {
        SDL_SetError("SDL_audiolib already initialized, cannot initialize again.");
        return false;
    }

    Stream_priv_device::fAudioSpec.freq = freq;
    Stream_priv_device::fAudioSpec.channels = channels;
    gInitType = InitType::NoOutput;
    std::atexit(Aulib::quit);
    return true;
}

void Aulib::quit()
{
    if (gInitType == InitType::None) {
        return;
    }
#if SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_CloseAudioDevice(Stream_priv_device::fDeviceId);
#else
    SDL_CloseAudio();
#endif
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    switch (Stream_priv_device::fDataType) {
    case BufferDataType::FLOAT:
        Stream_priv<float>::fSampleConverter = nullptr;
        break;
    case BufferDataType::INT32:
        Stream_priv<int32_t>::fSampleConverter = nullptr;
        break;
    }
    gInitType = InitType::None;
}

auto Aulib::sampleFormat() noexcept -> AudioFormat
{
    return Stream_priv_device::fAudioSpec.format;
}

auto Aulib::sampleRate() noexcept -> int
{
    return Stream_priv_device::fAudioSpec.freq;
}

auto Aulib::channelCount() noexcept -> int
{
    return Stream_priv_device::fAudioSpec.channels;
}

auto Aulib::frameSize() noexcept -> int
{
    return Stream_priv_device::fAudioSpec.samples;
}

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
