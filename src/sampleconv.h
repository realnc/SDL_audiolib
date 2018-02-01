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
#pragma once

#include "aulib_global.h"
#include <SDL_stdinc.h>

template <typename T>
class Buffer;

namespace Aulib {

/// \cond internal
AULIB_NO_EXPORT void floatToS8(Uint8 dst[], const Buffer<float>& src) noexcept;
AULIB_NO_EXPORT void floatToU8(Uint8 dst[], const Buffer<float>& src) noexcept;
AULIB_NO_EXPORT void floatToS16LSB(Uint8 dst[], const Buffer<float>& src) noexcept;
AULIB_NO_EXPORT void floatToU16LSB(Uint8 dst[], const Buffer<float>& src) noexcept;
AULIB_NO_EXPORT void floatToS16MSB(Uint8 dst[], const Buffer<float>& src) noexcept;
AULIB_NO_EXPORT void floatToU16MSB(Uint8 dst[], const Buffer<float>& src) noexcept;
AULIB_NO_EXPORT void floatToS32LSB(Uint8 dst[], const Buffer<float>& src) noexcept;
AULIB_NO_EXPORT void floatToS32MSB(Uint8 dst[], const Buffer<float>& src) noexcept;
AULIB_NO_EXPORT void floatToFloatLSB(Uint8 dst[], const Buffer<float>& src) noexcept;
AULIB_NO_EXPORT void floatToFloatMSB(Uint8 dst[], const Buffer<float>& src) noexcept;
/// \endcond internal

} // namespace Aulib
