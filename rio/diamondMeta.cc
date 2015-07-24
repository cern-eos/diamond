/* 
 * File:   diamondMeta.cc
 * Author: apeters
 * 
 * Created on October 15, 2014, 4:09 PM
 */

#include "diamondMeta.hh"

DIAMONDRIONAMESPACE_BEGIN

diamondMeta::diamondMeta (const diamond_ino_t ino, std::string name) { mIno = ino; mName = name; }

diamondMeta::diamondMeta (const diamondMeta& orig) { }

diamondMeta::diamondMeta (diamondMeta* orig) { mName = orig->getName(); mIno = orig->getIno(); memcpy(&mStat, orig->getStat(), sizeof (struct stat) ); }

diamondMeta::~diamondMeta () { }

void 
diamondMeta::setStat(struct stat& sbuf) 
{
  memcpy(&mStat, &sbuf, sizeof(struct stat));
}

void
diamondMeta::makeStat(uid_t uid, 
		      gid_t gid, 
		      ino_t ino,
		      mode_t mode,
		      off_t size)
{
  struct timespec ts;
  GetTimeSpecNow(ts);
  struct stat st;
  memset(&st, 0, sizeof(st));
  st.st_dev  = 0xcafe;
  st.st_ino  = ino;
  st.st_mode = mode;
  st.st_nlink= 1;  
  st.st_uid  = uid;
  st.st_gid  = gid;
  st.st_rdev = 0;
  st.st_size = size;
  st.st_blksize = 4096;
  st.st_blocks  = 0;
  st.st_atime   = ts.tv_sec;
  st.st_mtime   = ts.tv_sec;
  st.st_ctime   = ts.tv_sec;
  st.st_atim.tv_sec = ts.tv_sec;
  st.st_mtim.tv_sec = ts.tv_sec;
  st.st_ctim.tv_sec = ts.tv_sec;
  st.st_atim.tv_nsec = ts.tv_nsec;
  st.st_mtim.tv_nsec = ts.tv_nsec;
  st.st_ctim.tv_nsec = ts.tv_nsec;

  setStat(st);
}

DIAMONDRIONAMESPACE_END
