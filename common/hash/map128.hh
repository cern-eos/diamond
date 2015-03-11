// ----------------------------------------------------------------------
// File: map128.hh
// Author: Andreas-Joachim Peters - CERN
// ----------------------------------------------------------------------

/************************************************************************
 * DIAMOND - the CERN Disk Storage System                               *
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
 * @file   map128.hh
 *
 * @brief  Class implementing a lock free 128bit->128bit hash map
 *
 *
 */


#ifndef __DIAMONDCOMMON_MAP128_HH__
#define __DIAMONDCOMMON_MAP128_HH__

#include "common/Namespace.hh"
#include <sys/mman.h>

DIAMONDCOMMONNAMESPACE_BEGIN

class map128 {
public:

  __int128 _DELETED_;
  const __int128 _ZERO_ = 0;

  struct Entry {
    __int128 key;
    __int128 value;
  } __attribute__ ((aligned (16)));

private:
  Entry* m_entries;
  uint64_t m_arraySize;
  int mapfd;

  uint64_t m_item_cnt;
  uint64_t m_item_deleted_cnt;
  bool m_enable_cnt;

public:
  map128 (uint64_t arraySize, const char* mapefileName = 0, bool cnt = false);
  ~map128 ();

  // Basic operations
  bool SetItem (__int128 key, __int128 value, int syncflag = 0);

  void DeleteItem (__int128 key, int syncflag = 0);

  __int128 GetItem (__int128 key);
  uint64_t GetItemCount (bool effectively = true);
  void Clear ();
  int Sync (int syncflag);
  int Snapshot (const char* snapfileName, int syncflag = 0);

  void
  EnableCnt () {
    m_enable_cnt = true;
  }

  void
  DisableCnt () {
    m_enable_cnt = false;
  }
};

DIAMONDCOMMONNAMESPACE_END

#endif
