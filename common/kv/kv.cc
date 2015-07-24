// ----------------------------------------------------------------------
// File: kv.cc
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
 *           A                                                           *
 * You should have received a copy of the GNU General Public License    *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.*
 ************************************************************************/

/*----------------------------------------------------------------------------*/
#include "common/Namespace.hh"
#include "common/Logging.hh"
#include "common/hash/map128.hh"
#include "common/hash/spooky.hh"
#include "common/kv/kv.hh"
/*----------------------------------------------------------------------------*/
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <iostream>

/*----------------------------------------------------------------------------*/

DIAMONDCOMMONNAMESPACE_BEGIN


/*----------------------------------------------------------------------------*/
/**
 */
/*----------------------------------------------------------------------------*/

kv::kv (std::string kvdevice, std::string indexdirectory, uint64_t keysize) : LogId ()
{
  m_device.name = kvdevice;
  m_index_directory = indexdirectory;
  m_keysize = keysize;
}

kv::~kv ()
{
  Close();
}

int
kv::Close ()
{
  int retc = 0;
  if (m_device.mapping)
  {
    retc |= munmap(m_device.mapping, m_device.size);
    m_device.mapping = 0;
  }
  if (m_device.fd > 0)
  {
    retc |= close(m_device.fd);
    m_device.fd = 0;
  }

  retc |= SyncIndex(MS_ASYNC);

  return retc;
}

int
kv::Open (bool do_mmap)
{
  // open device, but don't create it

  m_device.fd = open(m_device.name.c_str(), O_RDWR);
  if (m_device.fd < 0)
  {
    diamond_err("msg=\"failed to open device\" device=\"%s\" errno=%d",
                m_device.name.c_str(), errno);
    return errno;
  }

  m_device.size = lseek(m_device.fd, 0, SEEK_END);
  if (m_device.size < 0)
  {
    diamond_err("msg=\"failed to determine device size\" device=\"%s\" errno=%d",
                m_device.name.c_str(), errno);
  }

  if (do_mmap)
  {
    m_device.mapping = mmap(0, m_device.size,
                            PROT_READ | PROT_WRITE, MAP_SHARED, m_device.fd, 0);

    if (m_device.mapping == MAP_FAILED)
    {
      diamond_err("msg=\"failed to map device\" device=\"%s\" errno=%d",
                  m_device.name.c_str(), errno);
    }
  }

  std::string m_index_file = m_index_directory + "/" + "kv.map128";
  if (!m_index)
  {
    diamond_info("msg=\"creating index\"");
    m_index = std::make_shared<map128>(m_keysize, m_index_file.c_str(), true);
  }
  if (do_mmap)
  {
    diamond_info("msg=\"mapped device\" device=\"%s\" size=%lld index-size=%lld",
                 m_device.name.c_str(),
                 m_device.size,
                 m_keysize);
  }
  else
  {
    diamond_info("msg=\"opened device\" device=\"%s\" size=%lld index-size=%lld",
                 m_device.name.c_str(),
                 m_device.size,
                 m_keysize);
  }

  bool nokey = false;

  // set the store stat structure
  m_stat.write_offset = m_index->GetItem(c_store_offset_key, nokey);
  if (nokey)
  {
    m_index->SetItem((__int128) c_store_offset_key, (__int128) 0, MS_SYNC);
  }
  m_stat.total_size = m_device.size;
  m_stat.used_size = m_stat.write_offset.load();
  m_stat.n_del = 0;
  m_stat.n_set = 0;
  m_stat.n_commit = 0;
  m_stat.n_sync = 0;
  m_stat.n_get = 0;
  m_stat.n_keys = m_index->GetItemCount(false);
  return 0;
}

void
kv::Status (std::ostream& os)
{
  m_stat.n_keys = m_index->GetItemCount(true);
  os << "+-----------------------------------------------------------------+\n";
  os << "| n_set                : " << m_stat.n_set.load() << "\n";
  os << "| n_commit             : " << m_stat.n_commit.load() << "\n";
  os << "| n_sync               : " << m_stat.n_sync.load() << "\n";
  os << "| n_get                : " << m_stat.n_get.load() << "\n";
  os << "| n_del                : " << m_stat.n_del.load() << "\n";
  os << "| n_keys               : " << m_stat.n_keys.load() << "\n";
  os << "| total size           : " << m_stat.total_size.load() << "\n";
  os << "| used  size           : " << m_stat.used_size.load() << "\n";
  os << "| write-offset         : " << m_stat.write_offset.load() << "\n";
  os << "+-----------------------------------------------------------------+\n";
}

/*----------------------------------------------------------------------------*/
int
kv::MakeFileStore (off_t storesize)
{
  int status = 0;
  {
    // make filestore parent
    std::ostringstream s;
    s << "mkdir -p " << " `dirname " << m_device.name << "`";
    status = system(s.str().c_str());
    if (WEXITSTATUS(status))
    {
      diamond_err("msg=\"mkpath failed\" path=\"%s\" rc=%d\n",
                  m_device.name.c_str(),
                  WEXITSTATUS(status));
      return WEXITSTATUS(status);
    }

    int fd = creat(m_device.name.c_str(), S_IRWXU);
    if ((fd < 0) && (errno != EEXIST))
    {
      diamond_err("msg=\"filestore creation failed\" errno=%d\n", errno);
      return errno;
    }

    if (ftruncate(fd, storesize))
    {
      close(fd);
      diamond_err("msg=\"filestore truncation failed\" errno=%d\n", errno);
      return errno;
    }

    close(fd);
  }
  {
    std::ostringstream s;
    // make indexdirectory
    s << "mkdir -p " << m_index_directory;
    status = system(s.str().c_str());
    if (WEXITSTATUS(status))
    {
      diamond_err("msg\"mkpath failed\" path=\"%s\" rc=%d\n",
                  m_device.name.c_str(),
                  WEXITSTATUS(status));
      return WEXITSTATUS(status);
    }
  }
  diamond_info("msg=\"configured filestore\" dev=\"%s\" size=%lld index=\"%s\"",
               m_device.name.c_str(),
               storesize,
               m_index_directory.c_str());

  return 0;
}

/*----------------------------------------------------------------------------*/

kv::kv_handle_t
kv::Set (std::string& key, void* value, size_t value_length)
{
  kv_handle_t handle;
  kv_item item;
  struct timespec ts;
  size_t key_length = key.length();

  // get a space pointer in the WAL store trying to advance the end offset
  uint64_t itemsize = item.size(key_length, value_length);
  bool new_offset;
  uint64_t myitemoffset;
  do
  {
    bool nokey;
    m_stat.write_offset = m_index->GetItem(c_store_offset_key, nokey);
    myitemoffset = m_stat.write_offset;
    new_offset = m_index->SetItem(c_store_offset_key,
                                  myitemoffset + itemsize,
                                  0,
                                  myitemoffset,
                                  true);
  }
  while (!new_offset);

  m_stat.write_offset += itemsize;

  std::cerr << "set " << myitemoffset << " length " << itemsize << std::endl;
  // get current time
  clock_gettime(CLOCK_REALTIME, &ts);
  if (m_device.mapping)
  {
    // place our item
    item.m_header = (kv_item_header_t*) ((char*) m_device.mapping + myitemoffset);

    item.m_header->m_magic = item.c_magic;
    item.m_header->m_ctime = ts.tv_sec;
    item.m_header->m_ctime_ns = ts.tv_nsec;
    item.m_header->m_crc32c = 0;
    item.m_header->m_size = itemsize;
    item.m_header->m_flags = item.c_none;
    item.m_header->m_key_length = key_length;
    item.m_header->m_value_length = value_length;
    item.m_header->m_reserved = 0;
    item.m_item.key = (char*) item.m_header + sizeof (kv_item_header_t);
    item.m_item.value = item.m_item.key + key_length;
    item.m_footer = (kv_item_footer_t*) (item.m_item.value + value_length);
    item.m_footer->m_magic = item.c_magic;
    item.m_footer->m_crc32c = 0;
  }

  // fill the handle
  handle.offset = (off_t) item.m_header - (off_t) m_device.mapping;
  handle.length = itemsize;
  handle.retc = 0;
  handle.value = item.m_item.value;

  m_stat.used_size += handle.length;

  // compute our hashed key and store in handle.key
  SpookyHash::Hash128(key.c_str(),
                      key_length,
                      (uint64_t*) & handle.key,
                      ((uint64_t*) & handle.key) + 1);

  if (m_device.mapping)
  {
    // copy the key
    memcpy(item.m_item.key, key.c_str(), key_length);
    if (value)
    {
      // copy the value if not peeking into it
      memcpy(item.m_item.value, value, value_length);
    }
  }
  else
  {
    struct iovec iov[4];
    int iov_cnt = 4;

    kv_item_header_t l_header;
    kv_item_footer_t l_footer;

    item.m_header = &l_header;
    item.m_footer = &l_footer;

    item.m_header->m_magic = item.c_magic;
    item.m_header->m_ctime = ts.tv_sec;
    item.m_header->m_ctime_ns = ts.tv_nsec;
    item.m_header->m_crc32c = 0;
    item.m_header->m_size = itemsize;
    item.m_header->m_flags = item.c_none;
    item.m_header->m_key_length = key_length;
    item.m_header->m_value_length = value_length;
    item.m_header->m_reserved = 0;
    item.m_item.key = (char*) key.c_str();
    item.m_item.value = (char*) value;
    item.m_footer->m_magic = item.c_magic;
    item.m_footer->m_crc32c = 0;

    iov[0].iov_base = (void*) &item.m_header;
    iov[1].iov_base = (void*) key.c_str();
    iov[2].iov_base = (void*) value;
    iov[3].iov_base = (void*) &item.m_footer;
    iov[0].iov_len = sizeof (kv_item_header_t);
    iov[1].iov_len = key_length;
    iov[2].iov_len = value_length;
    iov[3].iov_len = sizeof (kv_item_footer_t);

    ssize_t n_writev = pwritev(m_device.fd,
                               iov,
                               iov_cnt,
                               myitemoffset);

    if (errno || (n_writev != (ssize_t) itemsize))
    {
      handle.retc = errno;
      std::cerr << "fatal: wrote only " << n_writev << " bytes instead of "
        << itemsize << " - retc=" << handle.retc << " - aborting!" << std::endl;
      abort();
    }
  }

  m_stat.n_set++;

  return handle;
}

/*----------------------------------------------------------------------------*/
int
kv::Commit (const kv_handle_t& handle, int sync_flag)
{
  m_stat.n_commit++;
  return m_index->SetItem(handle.key, handle.offset, sync_flag);
}

/*----------------------------------------------------------------------------*/
int
kv::Sync (const kv_handle_t& handle, int sync_flag)
{
  m_stat.n_sync++;
  return msync((void*) handle.offset, handle.length, sync_flag);
}

/*----------------------------------------------------------------------------*/

kv::kv_handle_t
kv::Get (std::string& key, bool validate)
{
  kv_handle_t handle;
  bool nokey = false;

  // compute our hashed key and store in handle.key
  SpookyHash::Hash128(key.c_str(),
                      key.length(),
                      (uint64_t*) & handle.key,
                      ((uint64_t*) & handle.key) + 1);

  uint64_t offset = (uint64_t) m_index->GetItem(handle.key, nokey);

  if (nokey)
  {
    handle.retc = -ENOENT;
  }
  else
  {
    kv_item item;

    if (m_device.mapping)
    {
      handle.retc = item.grab((void*) (offset + (off_t) m_device.mapping), validate);
    }
    else
    {
      std::cerr << "reading item \n";
      handle.retc = item.read(m_device, offset, validate);
    }
    if (!handle.retc)
    {
      handle.offset = offset;
      handle.length = item.m_header->m_value_length;
      handle.value = item.m_item.value;

      std::cerr << "get " << offset << " length " << handle.length << std::endl;
      if (item.value_s.get())
      {
        std::cerr << "assigning item value shared buffer" << std::endl;
        handle.value_s = item.value_s;
      }
      if (item.key_s.get())
      {
        std::cerr << "assigning item key shared buffer" << std::endl;
        handle.key_s = item.key_s;
      }
    }
    else
    {
      std::cerr << "get failed retc=" << handle.retc << std::endl;
    }
  }
  m_stat.n_get++;

  return handle;
}

/*----------------------------------------------------------------------------*/
void
kv::Delete (std::string& key, int sync_flag)
{
  kv_handle_t handle;
  SpookyHash::Hash128(key.c_str(),
                      key.length(),
                      (uint64_t*) & handle.key,
                      ((uint64_t*) & handle.key) + 1);

  m_stat.n_del++;
  m_index->DeleteItem(handle.key, sync_flag);
}

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
int32_t
kv::kv_item::validate ()
{
  if (!m_header || !m_item.key || !m_item.value || !m_footer)
    return ENODATA;

  if (m_header->m_crc32c != m_footer->m_crc32c)
    return EPROTO;

  if ((m_header->m_magic != c_magic) ||
      (m_footer->m_magic != c_magic))
    return ENOMSG;

  if ((m_header->m_flags & c_crc32c) &&
      crc32c() != m_header->m_crc32c)
    return EDOM;
  return 0;
}

/*----------------------------------------------------------------------------*/
int32_t
kv::kv_item::grab (void* ptr, bool do_validate)
{
  // place the item pointers
  m_header = static_cast<kv_item_header_t*> (ptr);

  m_item.key = (char*) ptr + sizeof (kv_item_header_t);

  m_item.value = (char*) ptr +
    sizeof (kv_item_header_t) +
    m_header->m_key_length;

  m_footer = (kv_item_footer_t*) ((char*) ptr +
                                  sizeof (kv_item_header_t) +
                                  m_header->m_key_length +
                                  m_header->m_value_length);
  if (do_validate)
    return validate();
  else
    return 0;
}

/*----------------------------------------------------------------------------*/
int32_t
kv::kv_item::read (device_t &device, off_t offset, bool do_validate)
{
  kv_item_header_t l_header;
  kv_item_footer_t l_footer;

  m_header = &l_header;
  m_footer = &l_footer;

  // read the header
  size_t nread = ::pread(device.fd, &l_header, sizeof (l_header), offset);
  if (errno || (nread != sizeof (l_header)))
  {
    return errno;
  }
  // read key + value + footer
  struct iovec iov[3];
  int iov_cnt = 3;

  std::cerr << "key length " << l_header.m_key_length << " val length " << l_header.m_value_length << std::endl;
  // allocate shared buffers
  key_s = std::make_shared<kv_buffer> (l_header.m_key_length);
  value_s = std::make_shared<kv_buffer> (l_header.m_value_length);

  // attach memory buffers
  m_item.key = (char*) key_s.get();
  m_item.value = (char*) value_s.get();

  iov[0].iov_base = m_item.key;
  iov[0].iov_len = l_header.m_key_length;
  iov[1].iov_base = m_item.value;
  iov[1].iov_len = l_header.m_value_length;
  iov[2].iov_base = &l_footer;
  iov[2].iov_len = sizeof (l_footer);

  ssize_t n_pread = preadv(device.fd,
                           iov,
                           iov_cnt,
                           offset + sizeof (l_header));
  if (errno || ((size_t) n_pread != (iov[0].iov_len + iov[1].iov_len + iov[2].iov_len)))
  {
    return errno;
  }
  if (do_validate)
    return validate();

  return 0;
}

/*----------------------------------------------------------------------------*/
int64_t
kv::kv_item::size (size_t key_length, size_t value_length)
{
  return (
          sizeof (kv_item_header_t) +
          sizeof (kv_item_footer_t) +
          + key_length + value_length
          );
}
/*----------------------------------------------------------------------------*/
DIAMONDCOMMONNAMESPACE_END

