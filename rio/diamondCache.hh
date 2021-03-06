/*
 * File:   diamondCache.hh
 * Author: apeters
 *
 * Created on October 15, 2014, 4:09 PM
 */

#ifndef DIAMONDCACHE_HH
#define	DIAMONDCACHE_HH

#include <memory>
#include <map>
#include <list>
#include <sstream>

#include "rio/Namespace.hh"
#include "rio/diamondFile.hh"
#include "rio/diamondDir.hh"
#include "rio/diamond_types.hh"

#include "common/RWMutex.hh"
#include "common/Logging.hh"


DIAMONDRIONAMESPACE_BEGIN

using namespace diamond::common;

class diamondCache : LogId {
public:
  typedef std::shared_ptr<diamondFile> diamondFilePtr;
  typedef std::shared_ptr<diamondDir> diamondDirPtr;

  diamondFilePtr
  getFile (diamond_ino_t ino, bool update_lru=true, bool create=true, std::string name="");

  diamondDirPtr
  getDir (diamond_ino_t ino, bool update_lru=true, bool create=true, std::string name="");

  int
  rmFile (diamond_ino_t ino);
  
  int 
  rmDir (diamond_ino_t ino);


  diamondCache (std::string mount_point = "/");
  diamondCache (const diamondCache& orig);
  virtual ~diamondCache ();

  size_t fsize() {
    diamond::common::RWMutexReadLock flock(mFilesMutex);
    return mFilesLRU.size();
  }

  size_t dsize() {
    diamond::common::RWMutexReadLock flock(mDirsMutex);
    return mDirsLRU.size();
  }

  void DumpCachedFiles(std::stringstream& out);
  void DumpCachedDirs(std::stringstream& out);

  diamond_ino_t newInode() {
    static diamond::common::RWMutex sMutex;
    static unsigned long long last_inode=0;
    diamond::common::RWMutexWriteLock sLock(sMutex);
    return std::to_string(++last_inode);
  }

private:
  typedef std::pair<std::list<diamond_ino_t>::iterator, diamondFilePtr> lru_file_t;
  typedef std::pair<std::list<diamond_ino_t>::iterator, diamondDirPtr> lru_dir_t;

  typedef std::map< diamond_ino_t, lru_file_t > lru_file_map_t;
  typedef std::map< diamond_ino_t, lru_dir_t > lru_dir_map_t;
  typedef std::list< diamond_ino_t > lru_list_t;
  lru_file_map_t mFiles;
  lru_list_t mFilesLRU;
  diamond::common::RWMutex mFilesMutex;

  lru_dir_map_t mDirs;
  lru_list_t mDirsLRU;

  diamond::common::RWMutex mDirsMutex;


};

DIAMONDCOMMONNAMESPACE_END

#endif	/* DIAMONDCACHE_HH */


