// ----------------------------------------------------------------------
// File: BufferPtrLockFree.hh
// Author: Andreas-Joachim Peters - CERN
// ----------------------------------------------------------------------

/************************************************************************
 * DIAMOND - the CERN Disk Storage System                                   *
 * Copyright (C) 2015 CERN/Switzerland                                  *
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
 * @brief  Class implementing lock free buffer handling
 *
 *
 */


#ifndef __DIAMONDCOMMON_BUFFERLOCKFREE_HH__
#define __DIAMONDCOMMON_BUFFERLOCKFREE_HH__

#include "Namespace.hh"
#include "RWMutex.hh"
#include <memory>
#include <vector>
#include <string.h>

DIAMONDCOMMONNAMESPACE_BEGIN

class Bufferll_lf : public std::vector<char> {
public:

  Bufferll_lf (unsigned size = 0, unsigned capacity = 0) {
    if (size)
      resize(size);
    if (capacity)
      reserve(capacity);
  }

  virtual
  ~Bufferll_lf () { }

  //------------------------------------------------------------------------
  //! Add data
  //------------------------------------------------------------------------

  size_t
  putData (const void *ptr, size_t dataSize) {
    size_t currSize = size();
    resize(currSize + dataSize);
    memcpy(&operator[](currSize), ptr, dataSize);
    return dataSize;
  }

  off_t
  writeData (const void *ptr, off_t offset, size_t dataSize) {
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
  //! release related to peekData (empty)
  //------------------------------------------------------------------------

  void
  releasePeek () {
    
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

class BufferPtrLockFree {
public:

  BufferPtrLockFree (unsigned size = 0) {
    mBuffer = std::make_shared<Bufferll_lf> (size);
  }

  virtual
  ~BufferPtrLockFree () { }

  std::shared_ptr<Bufferll_lf> operator* () {
    return mBuffer;
  }
private:
  std::shared_ptr<Bufferll_lf> mBuffer;
};

DIAMONDCOMMONNAMESPACE_END

#endif

