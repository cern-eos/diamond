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

DIAMONDCOMMONNAMESPACE_BEGIN

class Bufferll
{
public:
  void* mPtr;
  bool mOwn;
  Bufferll(void* ptr=0, bool own=false) {
    mPtr = ptr;
    mOwn = own;
  }
  virtual ~Bufferll() {
    if (mPtr && mOwn) {
      free(mPtr);
    }
  }
  void* operator*() const  { return (void*) mPtr;}
};

class Buffer {
public:
  std::shared_ptr<Bufferll> mBuffer;
  Buffer(void* ptr=0, bool own=false) {
    mBuffer = std::make_shared<Bufferll> (ptr,own);
  }
  virtual ~Buffer() {}
  
  void* operator*() const  { return (void*) *(*mBuffer);}
};

DIAMONDCOMMONNAMESPACE_END
  
#endif

