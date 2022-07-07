// This is copyrighted software. More information is at the end of this file.
#pragma once
#include "aulib_config.h"

#if HAVE_STD_PRINT
#include <print>

namespace Aulib {
namespace priv {
using std::print;
using std::vprint;
} // namespace priv
} // namespace Aulib
#elif not HAVE_STD_FORMAT
#include <fmt/core.h>

namespace Aulib {
namespace priv {
using fmt::print;
using fmt::vprint;
} // namespace priv
} // namespace Aulib
#else
#include "missing/format.h"
#include <cstdio>

namespace Aulib {
namespace priv {

template <typename FormatString, typename... Args>
void print(FILE* stream, FormatString<Args...>&& fmt_str, Args&&... args)
{
    fprintf(
        stream,
        std::format(std::forward<FormatString>(fmt_str), std::forward<Args>(args)...).c_str());
}

} // namespace priv
} // namespace Aulib
#endif
/*

Copyright (C) 2021 Nikos Chantziaras.

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
