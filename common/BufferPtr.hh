// ----------------------------------------------------------------------
// File: Namespace.hh
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
#include <memory>
#include <vector>
#include <string.h>

DIAMONDCOMMONNAMESPACE_BEGIN

class Bufferll : public std::vector<char>
{
public:
  Bufferll(unsigned size) {
    resize(size);
  }
  virtual ~Bufferll() {
  }
  
  //------------------------------------------------------------------------
  //! Add data
  //------------------------------------------------------------------------
  void putData( const void *ptr, size_t dataSize ) {
    size_t currSize = size();
    resize( currSize + dataSize );
    memcpy( &operator[](currSize), ptr, dataSize );
  }

  //------------------------------------------------------------------------
  //! Add data
  //------------------------------------------------------------------------
  off_t grabData( off_t offset, void *ptr, size_t dataSize ) const {
    if( offset+dataSize > size() ) {
      return 0;
    }
    memcpy( ptr, &operator[](offset), dataSize );
    return offset+dataSize;
  }
};

class BufferPtr {
public:
  BufferPtr(unsigned size = 4096) {
    mBuffer = std::make_shared<Bufferll> (size);
  }
  virtual ~BufferPtr() {}
  
  std::shared_ptr<Bufferll>  operator*()   { return mBuffer; }
private:
  std::shared_ptr<Bufferll> mBuffer;

};

DIAMONDCOMMONNAMESPACE_END
  
#endif

