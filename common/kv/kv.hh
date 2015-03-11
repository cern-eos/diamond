// ----------------------------------------------------------------------
// File: kv.hh
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
 * @file   kv.hh
 *
 * @brief  Class implementing a lock free kv store
 *
 *
 */


#ifndef __DIAMONDCOMMON_KV_HH__
#define __DIAMONDCOMMON_KV_HH__

#include "common/Namespace.hh"
#include "common/BufferPtrLockFree.hh"
#include <sys/mman.h>
#include <stdint.h>
#include <atomic>

DIAMONDCOMMONNAMESPACE_BEGIN

class kv {
public:
  kv(std::string kvdevice, std::string indexdirectory, uint64_t keyspace);
  ~kv();

  int Init();

  void Status(std::ostream& os);

  typedef struct kv_stat 
  {
    std::atomic<uint64_t> n_set;
    std::atomic<uint64_t> n_get;
    std::atomic<uint64_t> n_del;
    std::atomic<uint64_t> total_size;
    std::atomic<uint64_t> used_size;
  } kv_stat_t;

  kv_stat_t m_stat;

  class kv_item {
  public:
    kv_item(){}
    ~kv_item(){}
  private:
    BufferPtrLockFree m_buf;
  };

private:
};

DIAMONDCOMMONNAMESPACE_END

#endif
