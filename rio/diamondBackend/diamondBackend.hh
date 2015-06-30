/*
 * File:   diamondBackend.hh
 * Author: apeters
 *
 * Created on April 21, 2015, 4:38 PM
 */

#ifndef DIAMONDBACKEND_HH
#define	DIAMONDBACKEND_HH

#include "rio/Namespace.hh"
#include "rio/diamondCache.hh"

DIAMONDRIONAMESPACE_BEGIN

class diamondBackend {
public:
  diamondBackend (std::string path);

  diamondBackend () { }
  diamondBackend (const diamondBackend& orig);
  virtual ~diamondBackend ();

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

};

DIAMONDRIONAMESPACE_END

#endif	/* DIAMONDBACKEND_HH */

