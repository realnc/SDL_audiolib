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
static T
floatSampleToInt(const float src) noexcept
{
    if (src >= 1.f) {
        return std::numeric_limits<T>::max();
    } else if (src < -1.f) {
        return std::numeric_limits<T>::min();
    } else {
        return src * static_cast<float>(1UL << (sizeof(T) * 8 - 1))
               + (static_cast<float>(1UL << (sizeof(T) * 8 - 1))
                  + static_cast<float>(std::numeric_limits<T>::min()));
    }
}


/* Convert float samples into integer samples.
 */
template<typename T>
static void
floatToInt(Uint8 dst[], const Buffer<float>& src) noexcept
{
    for (auto i : src) {
        T sample = floatSampleToInt<decltype(sample)>(i);
        memcpy(dst, &sample, sizeof(sample));
        dst += sizeof(sample);
    }
}


/* Convert float samples to endian-swapped integer samples.
 */
template<typename T>
static void
floatToSwappedInt(Uint8 dst[], const Buffer<float>& src) noexcept
{
    static_assert(sizeof(T) == 2 or sizeof(T) == 4, "");

    for (auto i : src) {
        T sample = floatSampleToInt<decltype(sample)>(i);
        switch (sizeof(T)) {
        case 4:
            *dst++ = *(Uint8*)((unsigned char*)&sample + 3);
            *dst++ = *(Uint8*)((unsigned char*)&sample + 2);
            // fall-through
        default:
            *dst++ = *(Uint8*)((unsigned char*)&sample + 1);
            *dst++ = *(Uint8*)((unsigned char*)&sample);
        }
    }
}


template<typename T>
static void
floatToLsbInt(Uint8 dst[], const Buffer<float>& src) noexcept
{
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    floatToInt<T>(dst, src);
#else
    floatToSwappedInt<T>(dst, src);
#endif
}


template<typename T>
static void
floatToMsbInt(Uint8 dst[], const Buffer<float>& src) noexcept
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    floatToInt<T>(dst, src);
#else
    floatToSwappedInt<T>(dst, src);
#endif
}


void
Aulib::floatToS8(Uint8 dst[], const Buffer<float>& src) noexcept
{
    floatToInt<Sint8>(dst, src);
}


void
Aulib::floatToU8(Uint8 dst[], const Buffer<float>& src) noexcept
{
    floatToInt<Uint8>(dst, src);
}


void
Aulib::floatToS16LSB(Uint8 dst[], const Buffer<float>& src) noexcept
{
    floatToLsbInt<Sint16>(dst, src);
}


void
Aulib::floatToU16LSB(Uint8 dst[], const Buffer<float>& src) noexcept
{
    floatToLsbInt<Uint16>(dst, src);
}


void
Aulib::floatToS16MSB(Uint8 dst[], const Buffer<float>& src) noexcept
{
    floatToMsbInt<Sint16>(dst, src);
}


void
Aulib::floatToU16MSB(Uint8 dst[], const Buffer<float>& src) noexcept
{
    floatToMsbInt<Uint16>(dst, src);
}


void
Aulib::floatToS32LSB(Uint8 dst[], const Buffer<float>& src) noexcept
{
    floatToLsbInt<Sint32>(dst, src);
}


void
Aulib::floatToFloat(Uint8 dst[], const Buffer<float>& src) noexcept
{
    memcpy(dst, src.get(), src.size() * sizeof(*src.get()));
}
