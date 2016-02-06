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
#include "boost/core/noncopyable.hpp"
#include "boost/scoped_array.hpp"
#include "aulib_debug.h"

/*! \private
 *
 * Simple RAII wrapper for buffers/arrays. More restrictive than std::vector.
 */
template <typename T>
class Buffer: private boost::noncopyable {
public:
    explicit Buffer(size_t size)
        : fData(new T[size]),
          fSize(size)
    { }

    ~Buffer()
    { }

    size_t size() const
    {
        return fSize;
    }

    void reset(size_t newSize)
    {
        fData.reset(new T[newSize]);
        fSize = newSize;
    }

    void resize(size_t newSize)
    {
        boost::scoped_array<T> newData(new T[newSize]);
        std::memcpy(newData.get(), fData.get(), sizeof(T) * std::min(newSize, fSize));
        fData.swap(newData);
        fSize = newSize;
    }

    T* data() const
    {
        return fData.get();
    }

private:
    boost::scoped_array<T> fData;
    size_t fSize;
};

#endif
