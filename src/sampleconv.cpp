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
#include "sampleconv.h"
#include <SDL_endian.h>
#include <SDL_version.h>
#include <limits>
#include "Buffer.h"


/* Convert and clip a float sample to an integer sample. This works for
 * all supported integer sample types (8-bit, 16-bit, 32-bit, signed or
 * unsigned.)
 */
template <typename T>
static void
floatSampleToInt(T& dst, float src) noexcept
{
    if (src >= 1.f) {
        dst = std::numeric_limits<T>::max();
    } else if (src < -1.f) {
        dst = std::numeric_limits<T>::min();
    } else {
        dst = src * (float)(1UL << (sizeof(T) * 8 - 1))
              + ((float)(1UL << (sizeof(T) * 8 - 1))
                 + (float)std::numeric_limits<T>::min());
    }
}


template <typename T>
static void
floatBufToInt(T dst[], const Buffer<float>& src) noexcept
{
    for (int i = 0; i < src.size(); ++i) {
        floatSampleToInt(dst[i], src[i]);
    }
}


void
Aulib::floatToS8(Uint8 dst[], const Buffer<float>& src) noexcept
{
    floatBufToInt((Sint8*)dst, src);
}


void
Aulib::floatToU8(Uint8 dst[], const Buffer<float>& src) noexcept
{
    floatBufToInt(dst, src);
}


void
Aulib::floatToS16LSB(Uint8 dst[], const Buffer<float>& src) noexcept
{
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    floatBufToInt((Sint16*)dst, src);
#else
    Sint16 sample;
    for (auto i : src) {
        floatSampleToInt(sample, i);
        *dst++ = *(Uint8*)((unsigned char*)&sample + 1);
        *dst++ = *(Uint8*)((unsigned char*)&sample);
    }
#endif
}


void
Aulib::floatToU16LSB(Uint8 dst[], const Buffer<float>& src) noexcept
{
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    floatBufToInt((Uint16*)dst, src);
#else
    Uint16 sample;
    for (auto i : src) {
        floatSampleToInt(sample, i);
        *dst++ = *(Uint8*)((unsigned char*)&sample + 1);
        *dst++ = *(Uint8*)((unsigned char*)&sample);
    }
#endif
}


void
Aulib::floatToS16MSB(Uint8 dst[], const Buffer<float>& src) noexcept
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    floatBufToInt((Sint16*)dst, src);
#else
    Sint16 sample;
    for (auto i : src) {
        floatSampleToInt(sample, i);
        *dst++ = *(Uint8*)((unsigned char*)&sample + 1);
        *dst++ = *(Uint8*)((unsigned char*)&sample);
    }
#endif
}


void
Aulib::floatToU16MSB(Uint8 dst[], const Buffer<float>& src) noexcept
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    floatBufToInt((Uint16*)dst, src);
#else
    Uint16 sample;
    for (auto i : src) {
        floatSampleToInt(sample, i);
        *dst++ = *(Uint8*)((unsigned char*)&sample + 1);
        *dst++ = *(Uint8*)((unsigned char*)&sample);
    }
#endif
}


void
Aulib::floatToS32LSB(Uint8 dst[], const Buffer<float>& src) noexcept
{
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    floatBufToInt((Sint32*)dst, src);
#else
    // FIXME: 4 bytes
    abort();
    Sint32 sample;
    for (auto i : src) {
        floatSampleToInt(sample, i);
        *dst++ = *(Uint8*)((unsigned char*)&sample + 1);
        *dst++ = *(Uint8*)((unsigned char*)&sample);
    }
#endif
}


void
Aulib::floatToFloat(Uint8 dst[], const Buffer<float>& src) noexcept
{
    memcpy(dst, src.get(), src.size() * sizeof(*src.get()));
}
