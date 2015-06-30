/*
 * File:   diamondBackend.cc
 * Author: apeters
 *
 * Created on April 21, 2015, 4:38 PM
 */

#include "rio/diamondCache.hh"
#include "diamondBackend.hh"

using namespace diamond::rio;

DIAMONDRIONAMESPACE_BEGIN

diamondBackend::diamondBackend(std::string path)
{
}

diamondBackend::diamondBackend(const diamondBackend& orig)
{
}

diamondBackend::~diamondBackend()
{
}

bool
diamondBackend::file_exists(diamond_ino_t ino, std::string& name)
{
  return false;
}

bool
diamondBackend::dir_exists(diamond_ino_t ino, std::string& name)
{
  return false;
}

int
diamondBackend::sync(diamondCache::diamondDirPtr& dir)
{
  return -1;
}

int
diamondBackend::sync(diamondCache::diamondFilePtr& dir)
{
  return -1;
}

int
diamondBackend::mkFile(uid_t uid, gid_t gid, mode_t mode,
  diamondCache::diamondDirPtr parent, std::string& name, diamond_ino_t& new_inode)
{
  return -1;
}

int
diamondBackend::mkDir(uid_t uid, gid_t gid, mode_t mode,
  diamondCache::diamondDirPtr parent, std::string& name, diamond_ino_t& new_inode)
{
  return -1;
}

int
diamondBackend::rmFile(uid_t uid, gid_t gid, diamond_ino_t rm_inode)
{
  return -1;
}

int
diamondBackend::rmDir(uid_t uid, gid_t gid, diamond_ino_t rm_inode)
{
  return -1;
}

int
diamondBackend::syncMeta(diamondMeta *meta, std::string path)
{
  return -1;
}

int
diamondBackend::renameFile(uid_t uid,
  gid_t gid,
  diamond_ino_t rename_inode,
  diamond_ino_t old_parent_inode,
  diamond_ino_t new_parent_inode,
  std::string old_name,
  std::string new_name)
{
  return -1;
}

int
diamondBackend::renameDir(uid_t uid,
  gid_t gid,
  diamond_ino_t rename_inode,
  diamond_ino_t old_parent_inode,
  diamond_ino_t new_parent_inode,
  std::string old_name,
  std::string new_name)
{
  return -1;
}

DIAMONDRIONAMESPACE_END