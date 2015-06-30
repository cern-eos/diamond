/*
 * File:   diamondMeta.hh
 * Author: apeters
 *
 * Created on October 15, 2014, 4:09 PM
 */

#ifndef DIAMONDMETA_HH
#define	DIAMONDMETA_HH

#include <string>
#include <map>
#include <set>
#include <list>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "rio/Namespace.hh"
#include "rio/diamond_types.hh"

#include "common/BufferPtr.hh"
#include "common/RWMutex.hh"

DIAMONDRIONAMESPACE_BEGIN

class diamondMeta : public std::map<std::string, diamond::common::BufferPtr> {
  friend class diamondFile;
  friend class diamondDir;
  friend class diamondCache;

public:

  typedef std::pair< std::string, std::string> action_t;

  const std::string kActionCommitXattr = "commit-xattr";
  const std::string kActionDeleteXattr = "delete-xattr";
  const std::string kActionFetchXattr = "fetch-xattr";
  const std::string kActionCommitUid = "commit-uid";
  const std::string kActionCommitGid = "commit-gid";
  const std::string kActionCommitMode = "commit-mode";
  const std::string kActionCommitSize = "commit-size";
  const std::string kActionCommitAtime = "commit-atime";
  const std::string kActionCommitMtime = "commit-mtime";
  const std::string kActionCommitCtime = "commit-ctime";
  const std::string kActionCommitAMtime = "commit-amtime";
  const std::string kActionCommitRenamed = "commit-rename";

  diamondMeta () { }
  diamondMeta (diamond_ino_t ino, std::string name);
  diamondMeta (const diamondMeta& orig);
  diamondMeta (diamondMeta* orig);
  virtual ~diamondMeta ();

  struct stat*
  getStat () {
    return &mStat;
  }

  struct timespec*
  getSyncTime () {
    return &mSyncTime;
  }

  void setStat (struct stat& sbuf);
  void makeStat (uid_t uid, gid_t gid, ino_t ino, mode_t mode, off_t size);

  diamond::common::RWMutex&
  Locker () {
    return mMutex;
  }

  diamond_ino_t&
  getIno () {
    return mIno;
  }

  std::string&
  getName () {
    return mName;
  }

  void
  setName (std::string name) {
    mName = name;
  }

  std::list<action_t>&
  getSyncOps () {
    return mSyncOps;
  }

  void
  addSyncOp (std::string name, std::string val) {
    diamondMeta::action_t action;
    action.first = name;
    action.second = val;
    diamond::common::RWMutexWriteLock mLock(Locker());
    mSyncOps.push_back(action);
  }

  std::list<action_t>&
  getAsyncOps () {
    return mAsyncOps;
  }

  void
  setSyncTime (struct stat& buf) {
    mSyncTime.tv_sec = buf.st_mtim.tv_sec;
    mSyncTime.tv_nsec = buf.st_mtim.tv_nsec;
  }

  void
  GetTimeSpecNow (struct timespec &ts) {
#ifdef __APPLE__
    struct timeval tv;
    gettimeofday(&tv, 0);
    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;
#else
    clock_gettime(CLOCK_REALTIME, &ts);
#endif
  }


protected:
  diamond_ino_t mIno;
  std::string mName;

  struct stat mStat;

  struct timespec mSyncTime;

  diamond::common::RWMutex mMutex;

  std::list< action_t> mSyncOps;
  std::list< action_t> mAsyncOps;
};

DIAMONDRIONAMESPACE_END

#endif	/* DIAMONDMETA_HH */

