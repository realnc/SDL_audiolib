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
#ifndef SDLAUDIOLOCKER_H
#define SDLAUDIOLOCKER_H

#include <SDL_audio.h>

/*! \private
 * RAII wrapper for SDL_LockAudio().
 */
class SdlAudioLocker {
public:
    SdlAudioLocker()
    {
        SDL_LockAudio();
        fIsLocked = true;
    }

    ~SdlAudioLocker()
    {
        unlock();
    }

    SdlAudioLocker(const SdlAudioLocker&) = delete;
    SdlAudioLocker& operator=(const SdlAudioLocker&) = delete;

    void unlock()
    {
        if (fIsLocked) {
            SDL_UnlockAudio();
            fIsLocked = false;
        }
    }

private:
    bool fIsLocked;
};

#endif
