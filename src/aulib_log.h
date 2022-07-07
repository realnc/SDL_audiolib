// This is copyrighted software. More information is at the end of this file.
#pragma once
#include "aulib_config.h"
#include "missing/format.h"
#include "missing/print.h"
#include "missing/string_view.h"
#include <cstdio>

namespace Aulib {
namespace priv {

inline void vlog(FILE* out, string_view prefix, string_view format, format_args&& args)
{
    fwrite(prefix.data(), 1, prefix.size(), out);
    vprint(out, format, args);
}

inline void vlogLn(FILE* out, string_view prefix, string_view format, format_args&& args)
{
    vlog(out, prefix, format, std::forward<format_args>(args));
    print(out, "\n");
}

} // namespace priv
} // namespace Aulib

namespace aulib {
namespace log {

template <typename... Args>
void debug([[maybe_unused]] Aulib::priv::string_view fmt_str, [[maybe_unused]] Args&&... args)
{
#if AULIB_DEBUG
    Aulib::priv::vlog(
        stderr, "SDL_audiolib debug: ", fmt_str, Aulib::priv::make_format_args(args...));
#endif
}

template <typename... Args>
void debugLn([[maybe_unused]] Aulib::priv::string_view fmt_str, [[maybe_unused]] Args&&... args)
{
#if AULIB_DEBUG
    Aulib::priv::vlogLn(
        stderr, "SDL_audiolib debug: ", fmt_str, Aulib::priv::make_format_args(args...));
#endif
}

template <typename... Args>
void warn(Aulib::priv::string_view fmt_str, Args&&... args)
{
    Aulib::priv::vlog(
        stderr, "SDL_audiolib warning: ", fmt_str, Aulib::priv::make_format_args(args...));
}

template <typename... Args>
void warnLn(Aulib::priv::string_view fmt_str, Args&&... args)
{
    Aulib::priv::vlogLn(
        stderr, "SDL_audiolib warning: ", fmt_str, Aulib::priv::make_format_args(args...));
}

template <typename... Args>
void info(Aulib::priv::string_view fmt_str, Args&&... args)
{
    Aulib::priv::vlog(
        stdout, "SDL_audiolib info: ", fmt_str, Aulib::priv::make_format_args(args...));
}

template <typename... Args>
void infoLn(Aulib::priv::string_view fmt_str, Args&&... args)
{
    Aulib::priv::vlogLn(
        stdout, "SDL_audiolib info: ", fmt_str, Aulib::priv::make_format_args(args...));
}

} // namespace log
} // namespace aulib

/*

Copyright (C) 2022 Nikos Chantziaras.

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
