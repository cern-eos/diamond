/*
 * File:   PosixBackend.hh
 * Author: apeters
 *
 * Created on April 22, 2015, 8:36 AM
 */

#ifndef POSIXBACKEND_HH
#define	POSIXBACKEND_HH

#include "diamondBackend.hh"
#include "rio/Namespace.hh"
#include "rio/diamondCache.hh"

DIAMONDRIONAMESPACE_BEGIN

class PosixBackend : public diamondBackend, LogId {
public:
  PosixBackend (std::string path);

  PosixBackend () { }
  PosixBackend (const PosixBackend& orig);
  virtual ~PosixBackend ();

  virtual bool file_exists (diamond_ino_t ino, std::string& name);
  virtual bool dir_exists (diamond_ino_t ino, std::string& name);
  virtual int sync (diamondCache::diamondDirPtr& dptr);
  virtual int sync (diamondCache::diamondFilePtr& fptr);
  virtual int mkFile (uid_t uid, gid_t gid, mode_t mode,
                      diamondCache::diamondDirPtr parent, std::string& name, diamond_ino_t& new_inode);
  virtual int mkDir (uid_t uid, gid_t gid, mode_t mode,
                     diamondCache::diamondDirPtr parent, std::string& name, diamond_ino_t& new_inode);

  virtual int rmFile (uid_t uid, gid_t gid, diamond_ino_t rm_inode);

  virtual int rmDir (uid_t uid, gid_t gid, diamond_ino_t rm_inode);

  virtual int syncMeta (diamondMeta *meta, std::string path);

  virtual int renameFile (uid_t uid,
                          gid_t gid,
                          diamond_ino_t rename_inode,
                          diamond_ino_t old_parent_inode,
                          diamond_ino_t new_parent_inode,
                          std::string old_name,
                          std::string new_name);

  virtual int renameDir (uid_t uid,
                         gid_t gid,
                         diamond_ino_t rename_inode,
                         diamond_ino_t old_parent_inode,
                         diamond_ino_t new_parent_inode,
                         std::string old_name,
                         std::string new_name);
private:
  std::string mRootPath;
  std::map<diamond_ino_t, std::string> mFileInodeMap;
  std::map<diamond_ino_t, std::string> mDirInodeMap;
  diamond::common::RWMutex mInodeMapMutex;

};

DIAMONDRIONAMESPACE_END

#endif	/* POSIXBACKEND_HH */

