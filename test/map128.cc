// ----------------------------------------------------------------------
// File: map128.cc
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
 * @file   map128.cc
 *
 * @brief  Google Test for the map128 class
 *
 *
 */

#include <cstdlib>
#include <iostream>
#include <sstream>

#include "gtest/gtest.h"
#include "common/Logging.hh"
#include "common/Timing.hh"
#include "common/hash/map128.hh"

using namespace diamond::common;

TEST (map128, mapset)
{
  diamond::common::Timing tm1("map128");
  map128 lkmap(1024 * 1024, 0, true);
  COMMONTIMING("creation", &tm1);

  EXPECT_EQ(0, lkmap.GetItemCount(true));
  EXPECT_EQ(0, lkmap.GetItemCount(false));

  COMMONTIMING("get-item-count", &tm1);

  bool result = true;
  __int128 key = 0x1234567887654321;
  __int128 val = 0xabcddcbaabcddcba;
  key <<= 64;
  val <<= 64;
  key += 0x1234567887654321;
  val += 0xabcddcbaabcddcba;
  if (!key)
    key++;
  if (!val)
    val++;
  for (size_t i = 0; i < 512 * 1024; i++)
  {
    result &= lkmap.SetItem(key, val);
    key++;
    val++;
  }
  COMMONTIMING("set-item", &tm1);

  EXPECT_EQ(true, result);
  EXPECT_EQ(512 * 1024, lkmap.GetItemCount(true));
  EXPECT_EQ(512 * 1024, lkmap.GetItemCount(false));
  COMMONTIMING("get-item-count", &tm1);
  EXPECT_EQ(0, lkmap.Snapshot("/tmp/map128.async.lkmap", MS_ASYNC));
  COMMONTIMING("snapshot-async", &tm1);
  EXPECT_EQ(0, lkmap.Snapshot("/tmp/map128.sync.lkmap", MS_SYNC));
  COMMONTIMING("snapshot-sync", &tm1);

  tm1.Print();
}

TEST (map128, mapoverflow)
{
  map128 lkmap(1024, 0, true);
  bool result = true;
  __int128 key = 0xabcdabcdabcdabcd;
  __int128 val = 0xcafecafecafecafe;
  for (size_t i = 0; i < 1024; i++)
  {
    key++;
    val++;
    result &= lkmap.SetItem(key, val);
  }
  EXPECT_EQ(true, result);
  for (size_t i = 0; i < 1; i++)
  {
    key++;
    val++;
    result &= lkmap.SetItem(key, val);
  }
  EXPECT_EQ(false, result);
  EXPECT_EQ(1024, lkmap.GetItemCount(true));
  EXPECT_EQ(1024, lkmap.GetItemCount(false));
}

TEST (map128, mapdelete)
{
  map128 lkmap(1024 * 1024, 0, true);
  EXPECT_EQ(0, lkmap.GetItemCount(true));
  EXPECT_EQ(0, lkmap.GetItemCount(false));

  bool result = true;
  {
    __int128 key = 0xabcdabcdabcdabcd;
    __int128 val = 0xcafecafecafecafe;
    for (size_t i = 0; i < 512 * 1024; i++)
    {
      key++;
      val++;
      result &= lkmap.SetItem(key, val);
    }
  }
  {
    __int128 key = 0xabcdabcdabcdabcd;
    __int128 val = 0xcafecafecafecafe;
    for (size_t i = 0; i < 512 * 1024; i++)
    {
      key++;
      val++;
      lkmap.DeleteItem(key);
    }
  }

  EXPECT_EQ(true, result);
  EXPECT_EQ(0, lkmap.GetItemCount(true));
  EXPECT_EQ(512 * 1024, lkmap.GetItemCount(false));
  EXPECT_EQ(0, lkmap.Snapshot("/tmp/map128.deleted.async.lkmap", MS_ASYNC));
}

TEST (map128, largemap)
{
  diamond::common::Timing tm1("largemap");
  map128 lkmap(32 * 1024 * 1024, 0, true);
  COMMONTIMING("creation", &tm1);

  EXPECT_EQ(0, lkmap.GetItemCount(true));
  EXPECT_EQ(0, lkmap.GetItemCount(false));

  COMMONTIMING("get-item-count", &tm1);

  bool result = true;
  __int128 key = 0x1234567887654321;
  __int128 val = 0xabcddcbaabcddcba;
  key <<= 64;
  val <<= 64;
  key += 0x1234567887654321;
  val += 0xabcddcbaabcddcba;
  if (!key)
    key++;
  if (!val)
    val++;
  for (size_t i = 0; i < 32 * 512 * 1024; i++)
  {
    result &= lkmap.SetItem(key, val);
    key++;
    val++;
  }
  COMMONTIMING("set-item", &tm1);

  EXPECT_EQ(true, result);
  EXPECT_EQ(32 * 512 * 1024, lkmap.GetItemCount(true));
  EXPECT_EQ(32 * 512 * 1024, lkmap.GetItemCount(false));
  COMMONTIMING("get-item-count", &tm1);
  EXPECT_EQ(0, lkmap.Snapshot("/tmp/map128.large.async.lkmap", MS_ASYNC));
  COMMONTIMING("snapshot-async", &tm1);
  EXPECT_EQ(0, lkmap.Snapshot("/tmp/map128.large.sync.lkmap", MS_SYNC));
  COMMONTIMING("snapshot-sync", &tm1);

  tm1.Print();
}
