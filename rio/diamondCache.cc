/*
 * File:   diamondCache.cc
 * Author: apeters
 *
 * Created on October 15, 2014, 4:09 PM
 */

#include "diamondCache.hh"
#include <iostream>
#include "diamondBackend/PosixBackend.hh"


diamond::rio::PosixBackend gBackend("/backend");

DIAMONDRIONAMESPACE_BEGIN

diamondCache::diamondCache(std::string mountpoint)
{
}

diamondCache::diamondCache(const diamondCache& orig)
{
}

diamondCache::~diamondCache()
{
}

diamondCache::diamondFilePtr
diamondCache::getFile(diamond_ino_t& ino, bool update_lru, std::string name)
{
  diamond::common::RWMutexWriteLock flock(mFilesMutex);

  if (mFiles.count(ino)) {
    // return an existing ino
    if (update_lru)
      mFilesLRU.splice(mFiles[ino].first, mFilesLRU, mFilesLRU.end());
  } else {
    if (!gBackend.file_exists(ino, name))
      return 0;

    // create a new one
    diamondFilePtr f = std::make_shared<diamondFile>(ino, name);
    mFilesLRU.push_front(ino);
    mFiles[ino] = std::make_pair(mFilesLRU.begin(), f);

    sync(f);
    // evt. shrink here
    return mFiles[ino].second;
  }
  sync(mFiles[ino].second);
  return mFiles[ino].second;
}

diamondCache::diamondDirPtr
diamondCache::getDir(diamond_ino_t& ino, bool update_lru, std::string name)
{
  diamondDirPtr dptr;
  {
    diamond::common::RWMutexWriteLock flock(mDirsMutex);

    if (mDirs.count(ino)) {
      // return an existing ino
      if (update_lru)
        mDirsLRU.splice(mDirs[ino].first, mDirsLRU, mDirsLRU.end());
    } else {
      // check if existing in backend
      if (!gBackend.dir_exists(ino, name))

        return 0;

      // create a new one
      diamondDirPtr d = std::make_shared<diamondDir>(ino, name);
      mDirsLRU.push_front(ino);
      mDirs[ino] = std::make_pair(mDirsLRU.begin(), d);
      sync(d);

      // evt. shrink here
      return mDirs[ino].second;
    }
    dptr = mDirs[ino].second;
  }

  sync(dptr);
  return dptr;
}

int
diamondCache::sync(diamondFilePtr& fptr)
{
  return gBackend.sync(fptr);
}

int
diamondCache::sync(diamondDirPtr& dptr)
{
  return gBackend.sync(dptr);
}

int
diamondCache::mkDir(uid_t uid, gid_t gid, mode_t mode, diamondDirPtr parent,
  diamond_ino_t& new_inode, std::string name)
{
  return gBackend.mkDir(uid, gid, mode, parent, name, new_inode);
}

int
diamondCache::mkFile(uid_t uid, gid_t gid, mode_t mode, diamondDirPtr parent,
  diamond_ino_t& new_inode, std::string name)
{
  return gBackend.mkFile(uid, gid, mode, parent, name, new_inode);
}

int
diamondCache::rmFile(uid_t uid, gid_t gid, diamond_ino_t ino)
{
  diamond::common::RWMutexWriteLock flock(mFilesMutex);
  int retc = 0;
  retc = gBackend.rmFile(uid, gid, ino);
  if (retc)
    return retc;

  if (mFiles.count(ino)) {
    mFilesLRU.erase(mFiles[ino].first);
    mFiles.erase(ino);
    return 0;
  }
  errno = ENOENT;
  return -1;
}

int
diamondCache::rmDir(uid_t uid, gid_t gid, diamond_ino_t ino)
{
  diamond::common::RWMutexWriteLock flock(mDirsMutex);
  int retc = 0;
  retc = gBackend.rmDir(uid, gid, ino);
  if (retc)
    return retc;

  if (mDirs.count(ino)) {
    mDirsLRU.erase(mDirs[ino].first);
    mDirs.erase(ino);
    return retc;
  }
  errno = ENOENT;
  return -1;
}

int
diamondCache::renameFile(uid_t uid, gid_t gid, diamond_ino_t ino,
  diamond_ino_t old_parent_ino, diamond_ino_t new_parent_ino,
  std::string old_name,
  std::string new_name)
{
  diamond::common::RWMutexWriteLock flock(mFilesMutex);
  int retc = 0;
  retc = gBackend.renameFile(uid, gid, ino, old_parent_ino,
    new_parent_ino, old_name, new_name);

  if (retc)
    return retc;

  if (mFiles.count(ino)) {
    return 0;
  }
  errno = ENOENT;
  return -1;
}

int
diamondCache::renameDir(uid_t uid, gid_t gid, diamond_ino_t ino,
  diamond_ino_t old_parent_ino, diamond_ino_t new_parent_ino,
  std::string old_name,
  std::string new_name)
{
  diamond::common::RWMutexWriteLock flock(mDirsMutex);
  int retc = 0;
  retc = gBackend.renameDir(uid, gid, ino, old_parent_ino,
    new_parent_ino, old_name, new_name);

  if (retc)
    return retc;

  if (mFiles.count(ino)) {
    return 0;
  }
  errno = ENOENT;
  return -1;
}

void
diamondCache::DumpCachedFiles(std::stringstream& out)
{
  diamond::common::RWMutexReadLock flock(mFilesMutex);
  for (auto it = mFilesLRU.begin(); it != mFilesLRU.end(); ++it) {
    out << *it;
    out << "\n";
  }
}

void
diamondCache::DumpCachedDirs(std::stringstream& out)
{
  diamond::common::RWMutexReadLock flock(mDirsMutex);
  for (auto it = mDirsLRU.begin(); it != mDirsLRU.end(); ++it) {
    out << *it;
    out << "\n";
  }
}

DIAMONDRIONAMESPACE_END
