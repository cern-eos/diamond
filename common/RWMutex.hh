// ----------------------------------------------------------------------
// File: RWMutex.hh
// Author: Andreas-Joachim Peters
// ----------------------------------------------------------------------

/************************************************************************
 * EOS - the CERN Disk Storage System                                   *
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
 * @file   RWMutex.hh
 *
 * @brief  Class implementing a fair read-write Mutex.
 */

#ifndef __DIAMONDCOMMON_RWMUTEX_HH__
#define __DIAMONDCOMMON_RWMUTEX_HH__

/*----------------------------------------------------------------------------*/
#include "common/Namespace.hh"
/*----------------------------------------------------------------------------*/
#include <stdio.h>
/*----------------------------------------------------------------------------*/
#define _MULTI_THREADED
#include <pthread.h>

/*----------------------------------------------------------------------------*/

DIAMONDCOMMONNAMESPACE_BEGIN


//! Class implements a fair rw mutex prefering writers
/*----------------------------------------------------------------------------*/
class RWMutex {
private:
  pthread_rwlock_t rwlock;
  pthread_rwlockattr_t attr;
  struct timespec wlocktime;
  struct timespec rlocktime;
  bool blocking;
  size_t readLockCounter;
  size_t writeLockCounter;

public:
  // ---------------------------------------------------------------------------
  //! Constructor
  // ---------------------------------------------------------------------------
  RWMutex ();

  // ---------------------------------------------------------------------------
  //! Destructor
  // ---------------------------------------------------------------------------
  ~RWMutex ();

  // ---------------------------------------------------------------------------
  //! Set the write lock to blocking or not blocking
  // ---------------------------------------------------------------------------
  void SetBlocking (bool block);

  // ---------------------------------------------------------------------------
  //! Set the time to wait the acquisition of the write mutex before releasing quicky and retrying
  // ---------------------------------------------------------------------------
  void SetWLockTime (const size_t &nsec);

  // ---------------------------------------------------------------------------
  //! Lock for read
  // ---------------------------------------------------------------------------
  void LockRead ();


  // ---------------------------------------------------------------------------
  //! Lock for read allowing to be canceled waiting for a lock
  // ---------------------------------------------------------------------------
  void LockReadCancel ();


  // ---------------------------------------------------------------------------
  //! Unlock a read lock
  // ---------------------------------------------------------------------------
  void UnLockRead ();

  // ---------------------------------------------------------------------------
  //! Lock for write
  // ---------------------------------------------------------------------------
  void LockWrite ();

  // ---------------------------------------------------------------------------
  //! Unlock a write lock
  // ---------------------------------------------------------------------------
  void UnLockWrite ();

  // ---------------------------------------------------------------------------
  //! Lock for write but give up after wlocktime
  // ---------------------------------------------------------------------------
  int TimeoutLockWrite ();
};

/*----------------------------------------------------------------------------*/
//! Class implementing a monitor for write locking

/*----------------------------------------------------------------------------*/
class RWMutexWriteLock {
private:
  RWMutex* Mutex;

public:
  // ---------------------------------------------------------------------------
  //! Constructor
  // ---------------------------------------------------------------------------
  RWMutexWriteLock (RWMutex &mutex);

  // ---------------------------------------------------------------------------
  //! Destructor
  // ---------------------------------------------------------------------------
  ~RWMutexWriteLock ();
};

/*----------------------------------------------------------------------------*/
//! Class implementing a monitor for read locking

/*----------------------------------------------------------------------------*/
class RWMutexReadLock {
private:
  RWMutex* Mutex;

public:
  // ---------------------------------------------------------------------------
  //! Constructor
  // ---------------------------------------------------------------------------
  RWMutexReadLock (RWMutex &mutex);

  RWMutexReadLock (RWMutex &mutex, bool allowcancel);
  // ---------------------------------------------------------------------------
  //! Destructor
  // ---------------------------------------------------------------------------
  ~RWMutexReadLock ();
};

/*----------------------------------------------------------------------------*/
DIAMONDCOMMONNAMESPACE_END

#endif

