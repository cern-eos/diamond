// ----------------------------------------------------------------------
// File: RWMutex.cc
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

/*----------------------------------------------------------------------------*/
#include "common/RWMutex.hh"
#include <errno.h>

/*----------------------------------------------------------------------------*/


DIAMONDCOMMONNAMESPACE_BEGIN

/*----------------------------------------------------------------------------*/
RWMutex::RWMutex()
{
  // ---------------------------------------------------------------------------
  //! Constructor
  // ---------------------------------------------------------------------------

  // by default we are not a blocking write mutex
  blocking = false;
  // try to get write lock in 5 seconds, then release quickly and retry
  wlocktime.tv_sec = 5;
  wlocktime.tv_nsec = 0;
  // try to get read lock in 100ms, otherwise allow this thread to be canceled - used by LockReadCancel
  rlocktime.tv_sec = 0;
  rlocktime.tv_nsec = 1000000;
  readLockCounter = writeLockCounter = 0;

#ifndef __APPLE__
  pthread_rwlockattr_init(&attr);

  // readers don't go ahead of writers!
  if (pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NP)) {
    throw "pthread_rwlockattr_setkind_np failed";
  }
  if (pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED)) {
    throw "pthread_rwlockattr_setpshared failed";
  }

  if ((pthread_rwlock_init(&rwlock, &attr))) {
    throw "pthread_rwlock_init failed";
  }
}
#else
  pthread_rwlockattr_init(&attr);
  if (pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED)) {
    throw "pthread_rwlockattr_setpshared failed";
  }
  if ((pthread_rwlock_init(&rwlock, &attr))) {
    throw "pthread_rwlock_init failed";
  }
}
#endif

RWMutex::~RWMutex()
{
  // ---------------------------------------------------------------------------
  //! Destructor
  // ---------------------------------------------------------------------------
}

void
RWMutex::SetBlocking(bool block)
{
  // ---------------------------------------------------------------------------
  //! Set the write lock to blocking or not blocking
  // ---------------------------------------------------------------------------

  blocking = block;
}

void
RWMutex::SetWLockTime(const size_t &nsec)
{
  // ---------------------------------------------------------------------------
  //! Set the time to wait the acquisition of the write mutex before releasing quicky and retrying
  // ---------------------------------------------------------------------------

  wlocktime.tv_sec = nsec / 1000000;
  wlocktime.tv_nsec = nsec % 1000000;
}

void
RWMutex::LockRead()
{
  // ---------------------------------------------------------------------------
  //! Lock for read
  // ---------------------------------------------------------------------------

  if (pthread_rwlock_rdlock(&rwlock)) {
    throw "pthread_rwlock_rdlock failed";
  }
}

void
RWMutex::LockReadCancel()
{
  // ---------------------------------------------------------------------------
  //! Lock for read allowing to be canceled waiting for a lock
  // ---------------------------------------------------------------------------

#ifndef __APPLE__
  while (1) {
    int rc = pthread_rwlock_timedrdlock(&rwlock, &rlocktime);
    if (rc) {
      if (rc == ETIMEDOUT) {
        fprintf(stderr, "=== READ LOCK CANCEL POINT == TID=%llu OBJECT=%llx\n", (unsigned long long) pthread_self(), (unsigned long long) this);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
        pthread_testcancel();

        struct timespec naptime, waketime;
        naptime.tv_sec = 0;
        naptime.tv_nsec = 100 * 1000000;
        while (nanosleep(&naptime, &waketime) && EINTR == errno) {
          naptime.tv_sec = waketime.tv_sec;
          naptime.tv_nsec = waketime.tv_nsec;
        }
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
      } else {
        fprintf(stderr, "=== READ LOCK EXCEPTION == TID=%llu OBJECT=%llx rc=%d\n", (unsigned long long) pthread_self(), (unsigned long long) this, rc);
        throw "pthread_rwlock_timedrdlock failed";
      }
    } else {
      break;
    }
  }
#else
  LockRead();
#endif
}

void
RWMutex::UnLockRead()
{
  // ---------------------------------------------------------------------------
  //! Unlock a read lock
  // ---------------------------------------------------------------------------

  if (pthread_rwlock_unlock(&rwlock)) {
    throw "pthread_rwlock_unlock failed";
  }
}

void
RWMutex::LockWrite()
{
  // ---------------------------------------------------------------------------
  //! Lock for write
  // ---------------------------------------------------------------------------

  if (blocking) {
    // a blocking mutex is just a normal lock for write
    if (pthread_rwlock_wrlock(&rwlock)) {
      throw "pthread_rwlock_wrlock failed";
    }
  } else {
#ifdef __APPLE__
    // -------------------------------------------------
    // Mac does not support timed mutexes
    // -------------------------------------------------
    if (pthread_rwlock_wrlock(&rwlock)) {
      throw "pthread_rwlock_rdlock failed";
    }
#else
    // a non-blocking mutex tries for few seconds to write lock, then releases
    // this has the side effect, that it allows dead locked readers to jump ahead the lock queue
    while (1) {
      int rc = pthread_rwlock_timedwrlock(&rwlock, &wlocktime);
      if (rc) {
        if (rc != ETIMEDOUT) {
          fprintf(stderr, "=== WRITE LOCK EXCEPTION == TID=%llu OBJECT=%llx rc=%d\n", (unsigned long long) pthread_self(), (unsigned long long) this, rc);
          throw "pthread_rwlock_wrlock failed";
        } else {
          //fprintf(stderr,"==== WRITE LOCK PENDING ==== TID=%llu OBJECT=%llx\n",(unsigned long long)pthread_self(), (unsigned long long)this);
          struct timespec naptime, waketime;
          naptime.tv_sec = 0;
          naptime.tv_nsec = 500 * 1000000;
          while (nanosleep(&naptime, &waketime) && EINTR == errno) {
            naptime.tv_sec = waketime.tv_sec;
            naptime.tv_nsec = waketime.tv_nsec;
          }
        }
      } else {
        //	  fprintf(stderr,"=== WRITE LOCK ACQUIRED  ==== TID=%llu OBJECT=%llx\n",(unsigned long long)pthread_self(), (unsigned long long)this);
        break;
      }
    }
#endif
  }
}

void
RWMutex::UnLockWrite()
{
  // ---------------------------------------------------------------------------
  //! Unlock a write lock
  // ---------------------------------------------------------------------------

  if (pthread_rwlock_unlock(&rwlock)) {
    throw "pthread_rwlock_unlock failed";
  }

}

int
RWMutex::TimeoutLockWrite()
{
  // ---------------------------------------------------------------------------
  //! Lock for write but give up after wlocktime
  // ---------------------------------------------------------------------------

#ifdef __APPLE__
  return pthread_rwlock_wrlock(&rwlock);
#else
  return pthread_rwlock_timedwrlock(&rwlock, &wlocktime);
#endif
}

RWMutexWriteLock::RWMutexWriteLock(RWMutex &mutex)
{
  // ---------------------------------------------------------------------------
  //! Constructor
  // ---------------------------------------------------------------------------

  Mutex = &mutex;
  Mutex->LockWrite();
}

RWMutexWriteLock::~RWMutexWriteLock()
{
  // ---------------------------------------------------------------------------
  //! Destructor
  // ---------------------------------------------------------------------------

  Mutex->UnLockWrite();
}

RWMutexReadLock::RWMutexReadLock(RWMutex &mutex)
{
  // ---------------------------------------------------------------------------
  //! Constructor
  // ---------------------------------------------------------------------------

  Mutex = &mutex;
  Mutex->LockRead();
}

RWMutexReadLock::RWMutexReadLock(RWMutex &mutex, bool allowcancel)
{
  // ---------------------------------------------------------------------------
  //! Constructor
  // ---------------------------------------------------------------------------

  if (allowcancel) {
    Mutex = &mutex;
    Mutex->LockReadCancel();
  } else {
    Mutex = &mutex;
    Mutex->LockRead();
  }
}

RWMutexReadLock::~RWMutexReadLock()
{
  // ---------------------------------------------------------------------------
  //! Destructor
  // ---------------------------------------------------------------------------

  Mutex->UnLockRead();
}

DIAMONDCOMMONNAMESPACE_END
