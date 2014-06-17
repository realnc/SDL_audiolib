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
#ifndef AULIB_DEBUG_H
#define AULIB_DEBUG_H

// FIXME: That's dumb. We should add actual print functions instead of wrapping the std streams.
#include <iostream>

#ifdef AULIB_DEBUG
    #include <cassert>
    #define AM_debugAssert assert
    #define AM_debugPrint(x) std::cerr << x
    #define AM_debugPrintLn(x) AM_debugPrint(x) << '\n'
#else
    #define AM_debugAssert(x)
    #define AM_debugPrintLn(x)
    #define AM_debugPrint(x)
#endif

#define AM_warn(x) std::cerr << x
#define AM_warnLn(x) std::cerr << x << '\n'

#endif
