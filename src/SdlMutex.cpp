// This is copyrighted software. More information is at the end of this file.
#include "SdlMutex.h"
#include "aulib_global.h"
#include <stdexcept>

SdlMutex::SdlMutex()
{
    if (not mutex_) {
        Aulib::priv::throw_(std::runtime_error(SDL_GetError()));
    }
}

void SdlMutex::lock()
{
    if (SDL_LockMutex(mutex_) != 0) {
        Aulib::priv::throw_(std::runtime_error(SDL_GetError()));
    }
}

void SdlMutex::unlock()
{
    if (SDL_UnlockMutex(mutex_) != 0) {
        Aulib::priv::throw_(std::runtime_error(SDL_GetError()));
    }
}

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
