// ----------------------------------------------------------------------
// File: rioCache.cc
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
 * @file   rioCache.cc
 *
 * @brief  Google Test for the rioCache class
 *
 *
 */

#include <cstdlib>
#include <iostream>
#include <sstream>

#include "gtest/gtest.h"
#include "common/Logging.hh"
#include "rio/diamondCache.hh"

using namespace diamond::common;
using namespace diamond::rio;

TEST(diamondCache, CacheLRU)
{
  Logging::Init();
  Logging::SetUnit("GTEST");
  Logging::SetLogPriority(LOG_DEBUG);

  diamondCache icache("/");

  EXPECT_EQ(icache.dsize(), 0);
  EXPECT_EQ(icache.fsize(), 0);

  for (size_t i = 0; i < 1000; i++) {
    icache.getFile(i);
    icache.getDir(i);
  }

  EXPECT_EQ(icache.dsize(), 1000);
  EXPECT_EQ(icache.fsize(), 1000);

  diamond_static_info("f-cache-size=%u d-cache-size=%u", icache.dsize(), icache.fsize());

  for (size_t i = 0; i < 1000; i++) {
    diamondCache::diamondFilePtr f = icache.getFile(i, false);
    diamondCache::diamondDirPtr d = icache.getDir(i, false);
  }

  std::stringstream sout;
  icache.DumpCachedFiles(sout);
  EXPECT_EQ(3890, sout.str().length());
}

