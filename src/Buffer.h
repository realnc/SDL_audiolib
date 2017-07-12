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
#ifndef BUFFER_H
#define BUFFER_H

#include <cstring>
#include <algorithm>
#include <memory>
#include "aulib_debug.h"

/*! \private
 *
 * Simple RAII wrapper for buffers/arrays. More restrictive than std::vector.
 */
template <typename T>
class Buffer final {
public:
    explicit Buffer(int size)
        : fData(std::make_unique<T[]>(size))
        , fSize(size)
    { }

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    int size() const noexcept
    {
        return fSize;
    }

    void reset(const int newSize)
    {
        if (newSize == fSize) {
            return;
        }
        fData = std::make_unique<T[]>(newSize);
        fSize = newSize;
    }

    void resize(const int newSize)
    {
        if (newSize == fSize) {
            return;
        }
        auto newData = std::make_unique<T[]>(newSize);
        std::memcpy(newData.get(), fData.get(), sizeof(T) * std::min(newSize, fSize));
        fData.swap(newData);
        fSize = newSize;
    }

    void swap(Buffer& other) noexcept
    {
        fData.swap(other.fData);
        std::swap(fSize, other.fSize);
    }

    // unique_ptr::operator[] is not noexcept, but in reality, it can't throw.
    T& operator [](const int pos) const noexcept
    {
        AM_debugAssert(pos < fSize);
        return fData[pos];
    }

    T* get() const noexcept
    {
        return fData.get();
    }

private:
    std::unique_ptr<T[]> fData;
    int fSize;
};

#endif
