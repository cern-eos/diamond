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
#include <memory>
#include <iostream>

DIAMONDCOMMONNAMESPACE_BEGIN

class kv : LogId {
public:

  // ---------------------------------------------------------------------------
  kv (std::string kvdevice,
      std::string indexdirectory,
      uint64_t keysize);

  ~kv ();
  // ---------------------------------------------------------------------------

  int Open (bool do_mmap = true);
  int Close ();

  int
  MakeFileStore (off_t storesize);

  // ---------------------------------------------------------------------------

  class kv_buffer {
  public:

    kv_buffer (size_t len) {
      m_buf = new char[len]();
    }

    ~kv_buffer () {
      std::cerr << "deleting buffer " << m_buf << std::endl;
      delete [] m_buf;
    }

    void*
    get () {
      return m_buf;
    }

  private:
    char* m_buf;
  };

  // ---------------------------------------------------------------------------

  typedef struct kv_handle {

    kv_handle() {
      offset = length = 0;
      retc = 0;
      key = 0;
      value = 0;
    }
    uint64_t offset;
    uint64_t length;
    int retc;
    __int128 key;
    char* value;
    std::shared_ptr < kv_buffer > key_s;
    std::shared_ptr < kv_buffer > value_s;
  } kv_handle_t;

  typedef struct device {

    device() {
      size = 0;
      fd = -1;
      mapping = 0;
    }
    std::string name;
    off_t size;
    int fd;
    void* mapping;
  } device_t;

  // ---------------------------------------------------------------------------
  kv_handle_t Set (std::string& key,
                   void* value,
                   const size_t value_length);

  kv_handle_t Get (std::string& key, bool validate = false);

  void Delete (std::string& key, int sync_flag);


  int
  Commit (const kv_handle_t& handle, int sync_flag);

  int
  Sync (const kv_handle_t& handle, int sync_flag);

  int
  SyncIndex (int sync_flag) {
    return m_index->Sync(sync_flag);
  }

  // ---------------------------------------------------------------------------
  void
  Status (std::ostream& os);

  const __int128 c_store_offset_key = 0xffeeddbbaaccff;


  // ---------------------------------------------------------------------------

  typedef struct kv_stat {
    std::atomic<uint64_t> n_set;
    std::atomic<uint64_t> n_commit;
    std::atomic<uint64_t> n_sync;
    std::atomic<uint64_t> n_get;
    std::atomic<uint64_t> n_del;
    std::atomic<uint64_t> total_size;
    std::atomic<uint64_t> used_size;
    std::atomic<uint64_t> write_offset;
    std::atomic<uint64_t> n_keys;
  } kv_stat_t;

  kv_stat_t m_stat;

  typedef struct kv_item_header {
    uint32_t m_magic;
    uint32_t m_ctime;
    uint32_t m_ctime_ns;
    uint32_t m_crc32c;
    uint32_t m_size;
    uint16_t m_flags;
    uint16_t m_key_length;
    uint32_t m_value_length;
    uint32_t m_reserved;
  } kv_item_header_t;

  typedef struct kv_item_offsets {
    char* key;
    char* value;
  } kv_item_kv_t;

  typedef struct kv_item_footer {
    uint32_t m_crc32c;
    uint32_t m_magic;
  } kv_item_footer_t;

  class kv_item {
  public:

    kv_item () {
      m_header = 0;
      m_item.key = 0;
      m_item.value = 0;
      m_footer = 0;
    }

    const uint32_t c_magic = 0xcafeaffe;
    const uint32_t c_crc32c = 0x00000001;
    const uint32_t c_lz4 = 0x00000002;
    const uint32_t c_none = 0x00000000;

    ~kv_item () { }

    kv_item_header_t* m_header;
    kv_item_kv_t m_item;
    kv_item_footer_t* m_footer;

    // optional storage for key/values
    std::shared_ptr < kv_buffer > key_s;
    std::shared_ptr < kv_buffer > value_s;

    int32_t validate ();

    int64_t size (size_t key_length, size_t value_length);

    int32_t grab (void* ptr, bool do_validate);

    int32_t read (device_t &device, off_t offset, bool do_validate);

    uint32_t
    crc32c () {
      return 0;
    }

  private:

  };

private:
  device_t m_device;
  std::string m_index_directory;
  std::string m_index_file;
  uint64_t m_keysize;
  std::shared_ptr<map128> m_index;
};

DIAMONDCOMMONNAMESPACE_END

#endif
