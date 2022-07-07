// This is copyrighted software. More information is at the end of this file.
#pragma once
#include "aulib_config.h"

#if HAVE_STD_FORMAT
#include <format>

namespace Aulib {
namespace priv {
using std::format;
using std::format_args;
using std::make_format_args;
} // namespace priv
} // namespace Aulib
#else
#include <fmt/core.h>

namespace Aulib {
namespace priv {
using fmt::format;
using fmt::format_args;
using fmt::make_format_args;
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
