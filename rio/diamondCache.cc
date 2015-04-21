/*
 * File:   diamondCache.cc
 * Author: apeters
 *
 * Created on October 15, 2014, 4:09 PM
 */

#include "diamondCache.hh"
#include <iostream>

DIAMONDRIONAMESPACE_BEGIN

diamondCache::diamondCache (std::string mountpoint) { }

diamondCache::diamondCache (const diamondCache& orig) { }

diamondCache::~diamondCache () { 
}



diamondCache::diamondFilePtr 
diamondCache::getFile(diamond_ino_t ino, bool update_lru, bool create, std::string name)
{
  diamond::common::RWMutexWriteLock flock(mFilesMutex);

  if (mFiles.count(ino)) {
    // return an existing ino
    if (update_lru)
      mFilesLRU.splice( mFiles[ino].first, mFilesLRU, mFilesLRU.end());
  } else {
    if (!create)
      return 0;

    // create a new one
    diamondFilePtr f = std::make_shared<diamondFile>(ino,name);
    mFilesLRU.push_front(ino);
    mFiles[ino] = std::make_pair( mFilesLRU.begin(), f );

    // evt. shrink here
  }
  return mFiles[ino].second;
}

diamondCache::diamondDirPtr 
diamondCache::getDir(diamond_ino_t ino, bool update_lru, bool create, std::string name)
{
  diamond::common::RWMutexWriteLock flock(mDirsMutex);

  if (mDirs.count(ino)) {
    // return an existing ino
    if (update_lru)
      mDirsLRU.splice( mDirs[ino].first, mDirsLRU, mDirsLRU.end());
  } else {
    if (!create)
      return 0;
    // create a new one
    diamondDirPtr d = std::make_shared<diamondDir>(ino,name);
    mDirsLRU.push_front(ino);
    mDirs[ino] = std::make_pair( mDirsLRU.begin(), d );

    // evt. shrink here
  }
  return mDirs[ino].second;
}

int
diamondCache::rmFile (diamond_ino_t ino)
{
  diamond::common::RWMutexWriteLock flock(mFilesMutex);

  if (mFiles.count(ino)) {
    mFilesLRU.erase(mFiles[ino].first);
    mFiles.erase(ino);

    return 0;
  }
  return ENOENT;
}

int
diamondCache::rmDir (diamond_ino_t ino)
{
  diamond::common::RWMutexWriteLock flock(mDirsMutex);

  if (mDirs.count(ino)) {
    mDirsLRU.erase(mDirs[ino].first);
    mDirs.erase(ino);
    return 0;
  }
  return ENOENT;
}

void 
diamondCache::DumpCachedFiles(std::stringstream& out)
{
  diamond::common::RWMutexReadLock flock(mFilesMutex);
  for ( auto it = mFilesLRU.begin(); it != mFilesLRU.end(); ++it ) {
    out << *it;
    out << "\n";
  }
}

void 
diamondCache::DumpCachedDirs(std::stringstream& out)
{
  diamond::common::RWMutexReadLock flock(mDirsMutex);
  for ( auto it = mDirsLRU.begin(); it != mDirsLRU.end(); ++it ) {
    out << *it;
    out << "\n";
  }
}

DIAMONDRIONAMESPACE_END
