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

class diamondMeta : public std::map<std::string, diamond::common::BufferPtr>  {
  friend class diamondFile;

public:
  diamondMeta () {}
  diamondMeta (diamond_ino_t ino, std::string name);
  diamondMeta (const diamondMeta& orig);
  diamondMeta (diamondMeta* orig);
  virtual ~diamondMeta ();

  struct stat* getStat() {return &mStat;}
  void setStat(struct stat& sbuf);
  void makeStat(uid_t uid, gid_t gid, ino_t ino, mode_t mode, off_t size);
  diamond::common::RWMutex& Locker() { return mMutex; }

  diamond_ino_t& getIno() {return mIno;}
  std::string& getName() {return mName;}
  void setName(std::string name) {mName = name;}

  void GetTimeSpecNow (struct timespec &ts) {
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
  diamond::common::RWMutex mMutex;

};

DIAMONDRIONAMESPACE_END

#endif	/* DIAMONDMETA_HH */

