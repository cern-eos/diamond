// ----------------------------------------------------------------------
// File: kv.cc
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
 * @file   kv.cc
 *
 * @brief  Google Test for the kv class
 *
 *
 */

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <thread>

#include "gtest/gtest.h"
#include "common/Logging.hh"
#include "common/Timing.hh"
#include "common/hash/map128.hh"
#include "common/hash/longstring.hh"
#include "common/kv/kv.hh"

using namespace diamond::common;

#define KEYS 11
#define THREADS 1

void
set_from_thread (size_t index, kv* store)
{
  std::string key;
  char buffer[128];
  for (size_t i = 0 + (index * KEYS); i < KEYS + (index * KEYS); ++i)
  {
    key = longstring::unsigned_to_decimal(i, buffer);

    if (!(i % 100000))
      std::cout << i << ":" << key.c_str() << std::endl;
    kv::kv_handle_t h = store->Set(key,
                                   (void*) key.c_str(),
                                   key.length() + 1
                                   );
    bool done = store->Commit(h, 0);
    if (!done)
    {
      std::cout << "error: failed to commit" << std::flush << std::endl;
    }
    store->Sync(h, MS_ASYNC);
  }
}

void
get_from_thread (size_t index, kv* store)
{
  std::string key;
  char buffer[128];
  for (size_t i = 0 + (index * KEYS); i < KEYS + (index * KEYS); ++i)
  {
    key = longstring::unsigned_to_decimal(i, buffer);

    kv::kv_handle_t h = store->Get(key,
                                   false
                                   );

    if (!(i % 100000))
      std::cout << i << " retc=" << h.retc << " key=" << key << " length=" << h.length << " value=" << h.value << std::flush << std::endl;

  }
}

TEST (kv, kvcreate)
{
  Logging::Init();
  Logging::SetUnit("GTEST");
  Logging::SetLogPriority(LOG_DEBUG);

  diamond::common::Timing tm1("kv");
  COMMONTIMING("kv-init", &tm1);
  kv store("/tmp/diamond-kv/cafe.kv", "/tmp/diamond-kv", 32ll * 1024 * 1024);
  EXPECT_EQ(0, store.MakeFileStore(4ll * 1024 * 1024 * 1024));
  COMMONTIMING("kv-created", &tm1);
  store.Open();
  COMMONTIMING("kv-initialized", &tm1);
  store.Status(std::cout);

  size_t num_threads = THREADS;

  std::thread t[num_threads];

  //Launch a group of threads
  for (size_t i = 0; i < num_threads; ++i)
  {
    t[i] = std::thread(set_from_thread, i, &store);
  }

  //Join the threads with the main thread
  for (size_t i = 0; i < num_threads; ++i)
  {
    t[i].join();
  }

  COMMONTIMING("kv-store-24M", &tm1);
  store.Status(std::cout);
  store.SyncIndex(MS_ASYNC);
  COMMONTIMING("kv-sync", &tm1);
  {
    std::string key;
    char buffer[128];
    for (size_t i = 0; i < num_threads * KEYS; ++i)
    {

      key = longstring::unsigned_to_decimal(i, buffer);

      kv::kv_handle_t h = store.Get(key,
                                    false
                                    );

      if (!(i % 100000))
        std::cout << i << " retc=" << h.retc << " key=" << key << " length=" << h.length << " value=" << h.value << std::flush << std::endl;
    }
  }
  COMMONTIMING("kv-read-st", &tm1);
  store.Status(std::cout);

  //Launch a group of threads
  for (size_t i = 0; i < num_threads; ++i)
  {
    t[i] = std::thread(get_from_thread, i, &store);
  }

  //Join the threads with the main thread
  for (size_t i = 0; i < num_threads; ++i)
  {
    t[i].join();
  }

  store.Close();
  store.Open(false);

  COMMONTIMING("kv-read-mt", &tm1);
  store.Status(std::cout);

  //Launch a group of threads
  for (size_t i = 0; i < num_threads; ++i)
  {
    t[i] = std::thread(get_from_thread, i, &store);
  }

  //Join the threads with the main thread
  for (size_t i = 0; i < num_threads; ++i)
  {
    t[i].join();
  }

  COMMONTIMING("kv-read-nomap-mt", &tm1);
  store.Status(std::cout);
  store.Close();
  // re-open memory mapped

  store.Open(true);
  {
    std::string key;
    char buffer[128];
    for (size_t i = 0; i < num_threads * KEYS; ++i)
    {

      key = longstring::unsigned_to_decimal(i, buffer);

      store.Delete(key, MS_ASYNC);

      if (!(i % 100000))
        std::cout << i << std::flush << std::endl;
    }
  }

  COMMONTIMING("kv-delete", &tm1);
  store.Status(std::cout);
  COMMONTIMING("kv-done", &tm1);
  tm1.Print();
}