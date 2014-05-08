// ----------------------------------------------------------------------
// File: BufferTest.cc
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
 * @file   BufferTest.cc
 * 
 * @brief  Google Test for the Buffer class
 * 
 * 
 */

#include <cstdlib>
#include "gtest/gtest.h"
#include "common/Logging.hh"
#include "common/Buffer.hh"

using namespace diamond::common;

TEST (Buffer, BufferZeroAlloc) {
  Logging::Init();
  Logging::SetUnit("GTEST");
  Logging::SetLogPriority(LOG_ERR);

  Buffer buffer(0,false);
  EXPECT_EQ((void*) *buffer, (void*) 0);
}

