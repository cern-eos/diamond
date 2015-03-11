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
#include "common/kv/kv.hh"
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

kv::kv(std::string kvdevice, std::string indexdirectory, uint64_t keyspace)
{
}

kv::~kv()
{
}

int 
kv::Init()
{
  return 0;
}

void Status(std::ostream& os)
{
}

/*----------------------------------------------------------------------------*/
DIAMONDCOMMONNAMESPACE_END

