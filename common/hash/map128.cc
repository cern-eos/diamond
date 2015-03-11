// ----------------------------------------------------------------------
// File: map128.cc
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
/*----------------------------------------------------------------------------*/
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/*----------------------------------------------------------------------------*/

DIAMONDCOMMONNAMESPACE_BEGIN


/*----------------------------------------------------------------------------*/
/**
 */
/*----------------------------------------------------------------------------*/

inline static uint64_t
integerHash (__int128 h)
{
  h ^= h >> 33;
  h *= 0xff51afd7ed558ccd;
  h ^= h >> 33;
  h *= 0xc4ceb9fe1a85ec53;
  h ^= h >> 33;
  h *= 0xff51afd7ed558ccd;
  h ^= h >> 33;
  h *= 0xc4ceb9fe1a85ec53;
  h ^= h >> 33;
  return (h & 0xffffffffffffffff);
}

map128::map128 (uint64_t arraySize, const char* mapfilename, bool cnt)
{
  // Initialize cells
  assert((arraySize & (arraySize - 1)) == 0); // Must be a power of 2
  m_arraySize = arraySize;
  m_entries = 0;
  m_enable_cnt = cnt;
  mapfd = 0;

  _DELETED_ =  (0xffffffffffffffff);
  _DELETED_ <<=64;
  _DELETED_ |= (0xffffffffffffffff);

  if (mapfilename)
  {
    mapfd = open(mapfilename, O_RDWR);
    assert(mapfd > 0);
    void *mapping;
    mapping = mmap(0, sizeof (Entry) * arraySize,
                   PROT_READ | PROT_WRITE, MAP_SHARED, mapfd, 0);
    assert(mapping != MAP_FAILED);
    m_entries = (Entry*) mapping;
  }
  else
  {
    m_entries = new Entry[arraySize];
  }
  Clear();
}

map128::~map128 ()
{
  if (mapfd > 0)
  {
    // Unmap cells
    if (m_entries)
      munmap(m_entries, sizeof (Entry) * m_arraySize);
    m_entries = 0;
    close(mapfd);
  }
  else
  {
    // Delete cells
    delete[] m_entries;
  }
}


// Basic operations

bool
map128::SetItem (__int128 key, __int128 value, int syncflag)
{
  assert(key != 0);
  assert(value != 0);
  assert(key != _DELETED_);

  size_t l_stopper = m_arraySize << 1;

  for (uint64_t idx = integerHash(key); l_stopper != 0; idx++, l_stopper--)
  {
    bool new_key = false;
    idx &= m_arraySize - 1;
    // Load the key that was there.
    __int128 probedKey = __atomic_load_n(&m_entries[idx].key, __ATOMIC_RELAXED);

    if (probedKey != key)
    {
      // -----------------------------------------------------------------------
      // The entry was either free, or contains another key.
      // -----------------------------------------------------------------------
      if ((probedKey != 0) && (probedKey != _DELETED_))
      {
        continue; // Usually, it contains another key. Keep probing.
      }
      // -----------------------------------------------------------------------
      // The entry was free. Now let's try to take it using a CAS.
      // -----------------------------------------------------------------------
      if (!__atomic_compare_exchange(&m_entries[idx].key, (probedKey != _DELETED_)?&_ZERO_:&_DELETED_, &key, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
      {
        // ---------------------------------------------------------------------
        // it was taken, let's see if by chance with the same key
        // ---------------------------------------------------------------------
        probedKey = __atomic_load_n(&m_entries[idx].key, __ATOMIC_RELAXED);
        if (probedKey != key)
        {
          continue; // Another thread just stole it from underneath us.
        }
      }
      else
      {
        // ---------------------------------------------------------------------
        // a new key has been set
        // ---------------------------------------------------------------------
        new_key = true;
      }
    }

    // ---------------------------------------------------------------------
    // Store the value in this array entry.
    // ---------------------------------------------------------------------
    __atomic_store(&m_entries[idx].value, &value, __ATOMIC_RELAXED);

    // ---------------------------------------------------------------------
    // Count items only if they are 'new'
    // ---------------------------------------------------------------------
    if (new_key && m_enable_cnt)
    {
      if ( probedKey != _DELETED_ )
        __atomic_fetch_add(&m_item_cnt, 1, __ATOMIC_SEQ_CST);
      else
        __atomic_fetch_sub(&m_item_deleted_cnt, 1, __ATOMIC_SEQ_CST);
    }

    if (mapfd && syncflag)
      msync(&m_entries[idx], sizeof (Entry), syncflag);
    return true;
  }
  return false;
}

void
map128::DeleteItem (__int128 key, int syncflag)
{
  size_t l_stopper = m_arraySize << 1;
  for (uint64_t idx = integerHash(key); l_stopper != 0; idx++, l_stopper--)
  {
    idx &= m_arraySize - 1;
    // Load the key that was there.
    __int128 probedKey = __atomic_load_n(&m_entries[idx].key, __ATOMIC_RELAXED);
    if (probedKey != key)
    {
      if (probedKey == 0)
        return;
      continue;
    }

    if (__atomic_compare_exchange(&m_entries[idx].key, &probedKey, &_DELETED_, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) 
    {
      if (m_enable_cnt) 
      {
	__atomic_fetch_add(&m_item_deleted_cnt, 1, __ATOMIC_SEQ_CST);
      }
    }
  }
}

__int128
map128::GetItem (__int128 key)
{
  assert(key != 0);

  for (uint64_t idx = integerHash(key);; idx++)
  {
    idx &= m_arraySize - 1;

    __int128_t probedKey = __atomic_load_n(&m_entries[idx].key, __ATOMIC_RELAXED);
    if (probedKey == key)
      return __atomic_load_n(&m_entries[idx].value, __ATOMIC_RELAXED);
    if (probedKey == 0)
      return 0;
  }
}

uint64_t
map128::GetItemCount (bool effectively)
{
  uint64_t itemCount = 0;
  if (effectively)
  {
    uint64_t itemDeletedCount = 0;
    itemCount = __atomic_load_n(&m_item_cnt, __ATOMIC_RELAXED);
    itemDeletedCount = __atomic_load_n(&m_item_deleted_cnt, __ATOMIC_RELAXED);
    return itemCount - itemDeletedCount;
  }
  for (uint64_t idx = 0; idx < m_arraySize; idx++)
  {
    if ((__atomic_load_n(&m_entries[idx].key, __ATOMIC_RELAXED) != 0)
        && (__atomic_load_n(&m_entries[idx].value, __ATOMIC_RELAXED) != 0))
      itemCount++;
  }
  return itemCount;
}

void
map128::Clear ()
{
  for (uint64_t idx = 0; idx < m_arraySize; idx++)
  {
    __atomic_store_n(&m_entries[idx].key, _ZERO_, __ATOMIC_RELAXED);
    __atomic_store_n(&m_entries[idx].value, _ZERO_, __ATOMIC_RELAXED);
  }
  __atomic_store_n(&m_item_cnt, 0, __ATOMIC_RELAXED);
  __atomic_store_n(&m_item_deleted_cnt, 0, __ATOMIC_RELAXED);
}

int
map128::Sync (int syncflag)
{
  return msync(m_entries, sizeof (Entry) * m_arraySize, syncflag);
}

int
map128::Snapshot (const char* snapfileName, int syncflag)
{
  int snapfd = open(snapfileName, O_RDWR | O_CREAT, S_IRWXU);
  if (snapfd > 0)
  {
    if (ftruncate(snapfd, sizeof (Entry) * m_arraySize))
    {
      return -1;
    }

    void *mapping;
    mapping = mmap(0, sizeof (Entry) * m_arraySize, PROT_READ | PROT_WRITE,
                   MAP_SHARED, snapfd, 0);
    if (mapping == MAP_FAILED)
    {
      return -1;
    }
    Entry* snapentry = (Entry*) mapping;

    for (uint64_t idx = 0; idx < m_arraySize; idx++)
    {
      __int128 key = __atomic_load_n(&m_entries[idx].key, __ATOMIC_RELAXED);
      __int128 val = __atomic_load_n(&m_entries[idx].value, __ATOMIC_RELAXED);
      if (val != _DELETED_)
      {
        memcpy(&snapentry->key, &key, sizeof (__int128));
        memcpy(&snapentry->value, &val, sizeof (__int128));
        snapentry++;
      }
      else
      {
        snapentry++;
      }
    }
    if (msync(mapping, sizeof (Entry) * m_arraySize, syncflag))
      return -1;
    if (munmap(mapping, sizeof (Entry) * m_arraySize))
      return -1;
    if (close(snapfd))
      return -1;
    return 0;
  }
  return -1;
}



/*----------------------------------------------------------------------------*/
DIAMONDCOMMONNAMESPACE_END

