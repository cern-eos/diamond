// ----------------------------------------------------------------------
// File: Buffer.hh
// Author: Andreas-Joachim Peters - CERN
// ----------------------------------------------------------------------

/************************************************************************
 * DIAMOND - the CERN Disk Storage System                                   *
 * Copyright (C) 2011 CERN/Switzerland                                  *
 *                                                                      *
 * This program is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.*
 ************************************************************************/

/**
 * @file   Buffer.hh
 *
 * @brief  Class implementing buffer handling
 *
 *
 */


#ifndef __DIAMONDCOMMON_BUFFER_HH__
#define __DIAMONDCOMMON_BUFFER_HH__

#include "Namespace.hh"
#include "RWMutex.hh"
#include <memory>
#include <vector>
#include <string.h>

DIAMONDCOMMONNAMESPACE_BEGIN

class Bufferll : public std::vector<char> {
public:

  Bufferll (unsigned size = 0, unsigned capacity = 0) {
    if (size)
      resize(size);
    if (capacity)
      reserve(capacity);
  }

  virtual
  ~Bufferll () { }

  //------------------------------------------------------------------------
  //! Add data
  //------------------------------------------------------------------------

  size_t
  putData (const void *ptr, size_t dataSize) {
    RWMutexWriteLock dLock(mMutex);
    size_t currSize = size();
    resize(currSize + dataSize);
    memcpy(&operator[](currSize), ptr, dataSize);
    return dataSize;
  }

  off_t
  writeData (const void *ptr, off_t offset, size_t dataSize) {
    RWMutexWriteLock dLock(mMutex);
    size_t currSize = size();
    if ((offset + dataSize) > currSize) {
      currSize = offset + dataSize;
      resize(currSize);
      if (currSize > capacity()) {
        reserve(currSize + (256 * 1024));
      }
    }
    memcpy(&operator[](offset), ptr, dataSize);
    return currSize;
  }

  //------------------------------------------------------------------------
  //! Retrieve data
  //------------------------------------------------------------------------

  size_t
  readData (void *ptr, off_t offset, size_t dataSize) {
    RWMutexReadLock dLock(mMutex);
    if (offset + dataSize > size()) {
      return 0;
    }
    memcpy(ptr, &operator[](offset), dataSize);
    return dataSize;
  }

  //------------------------------------------------------------------------
  //! peek data ( one has to call release claim aftewards )
  //------------------------------------------------------------------------

  size_t
  peekData (char* &ptr, off_t offset, size_t dataSize) {
    mMutex.LockRead();
    ptr = &(operator[](0)) + offset;
    int avail = size() - offset;
    if (((int) dataSize > avail)) {
      if (avail > 0)
        return avail;
      else
        return 0;
    }
    return dataSize;
  }

  //------------------------------------------------------------------------
  //! release a lock related to peekData
  //------------------------------------------------------------------------

  void
  releasePeek () {
    mMutex.UnLockRead();
  }

  //------------------------------------------------------------------------
  //! truncate a buffer
  //------------------------------------------------------------------------

  void
  truncateData (off_t offset) {
    resize(offset);
    reserve(offset);
  }

private:
  RWMutex mMutex;
};

class BufferPtr {
public:

  BufferPtr (unsigned size = 0) {
    mBuffer = std::make_shared<Bufferll> (size);
  }

  virtual
  ~BufferPtr () { }

  std::shared_ptr<Bufferll> operator* () {
    return mBuffer;
  }
private:
  std::shared_ptr<Bufferll> mBuffer;
};

DIAMONDCOMMONNAMESPACE_END

#endif

