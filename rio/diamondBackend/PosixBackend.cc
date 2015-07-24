/*
 * File:   PosixBackend.cc
 * Author: apeters
 *
 * Created on April 22, 2015, 8:36 AM
 */

#include <dirent.h>
#include <unistd.h>
#include <sys/fsuid.h>
#include <sys/types.h>
#include <attr/xattr.h>
#include "rio/diamondCache.hh"
#include "PosixBackend.hh"

using namespace diamond::rio;

DIAMONDRIONAMESPACE_BEGIN

PosixBackend::PosixBackend(std::string path)
{
  mRootPath = path;
}

PosixBackend::PosixBackend(const PosixBackend& orig)
{
}

PosixBackend::~PosixBackend()
{
}

bool
PosixBackend::file_exists(diamond_ino_t ino, std::string& name)
{
  diamond_static_info("ino=%llx name=%s", ino, name.c_str());


  diamond::common::RWMutexReadLock iLock(mInodeMapMutex);
  diamond_static_info("exists=%d",
    mFileInodeMap.count(ino));
  bool exists = mFileInodeMap.count(ino);
  if (exists) {
    std::string path = mFileInodeMap[ino];
    // extract base name
    name = path.substr(path.find_last_of("/\\") + 1);
  }
  return exists;
}

bool
PosixBackend::dir_exists(diamond_ino_t ino, std::string& name)
{
  diamond_static_info("ino=%llx name=%s", ino, name.c_str());

  if (ino == 1) {
    struct stat buf;
    if (!stat(mRootPath.c_str(), &buf))
      return true;
    return false;
  } else {
    diamond::common::RWMutexReadLock iLock(mInodeMapMutex);
    diamond_static_info("exists=%d",
      mDirInodeMap.count(ino));
    bool exists = mDirInodeMap.count(ino);
    if (exists) {
      std::string path = mDirInodeMap[ino];
      // extract base name
      name = path.substr(path.find_last_of("/\\") + 1);
    }
    return exists;
  }
}

int
PosixBackend::sync(diamondCache::diamondDirPtr& dir)
{
  int retc = 0;
  bool resync_listing = false;
  bool update_stats = false;

  diamond_static_info("ino=%llx name=%s",
    dir.get()->getIno(),
    dir.get()->getName().c_str()
    );

  std::string lDirPath;
  if (dir.get()->getIno() == 1) {
    lDirPath = mRootPath;
  } else {
    diamond::common::RWMutexReadLock iLock(mInodeMapMutex);
    if (!mDirInodeMap.count(dir.get()->getIno())) {
      errno = ENOENT;
      return -1;
    }
    lDirPath = mDirInodeMap[dir.get()->getIno()];
  }

  struct stat buf;

  {
    diamond::common::RWMutexReadLock dLock(dir.get()->Locker());
    {

      if ((dir.get()->getIno() == 1)) {
        diamond::common::RWMutexWriteLock iLock(mInodeMapMutex);

        if (!mDirInodeMap.count(dir.get()->getIno())) {
          mDirInodeMap[dir.get()->getIno()] = mRootPath;
        }
      }
    }

    if (!lstat(lDirPath.c_str(), &buf)) {
      if (!(dir.get()->getSyncOps().size())) {
        diamond_static_info("%d.%d %d.%d",
          buf.st_mtim.tv_sec,
          buf.st_mtim.tv_nsec,
          dir.get()->getSyncTime()->tv_sec,
          dir.get()->getSyncTime()->tv_nsec);
        if ((buf.st_mtim.tv_sec == dir.get()->getSyncTime()->tv_sec) &&
          (buf.st_mtim.tv_nsec == dir.get()->getSyncTime()->tv_nsec)) {
          // no back-end changes
          diamond_static_info("dir-fast-track:no-backend-changes ino=%llx name=%s", dir.get()->getIno(),
            dir.get()->getName().c_str());
          return 0;
        }
        dir.get()->setStat(buf);
        resync_listing = true;
      } else
        if ((buf.st_mtim.tv_sec >= dir.get()->getSyncTime()->tv_sec) &&
        (buf.st_mtim.tv_nsec > dir.get()->getSyncTime()->tv_nsec)) {
        // only front-end changes
        diamond_static_info("dir-fast-track:front-end-changes ino=%llx name=%s", dir.get()->getIno(),
          dir.get()->getName().c_str());
      }
    }
    if (S_ISLNK(buf.st_mode)) {
      diamond_static_info("msg=\"reading link\" path=%s", lDirPath.c_str());
      char linkpath[4096];
      ssize_t linksize = 0;
      if ((linksize = readlink(lDirPath.c_str(), linkpath, sizeof(linkpath))) > 0) {
        {
          linkpath[linksize] = 0;
          diamond::common::BufferPtr buffer;
          buffer = dir.get()->std::map<std::string, diamond::common::BufferPtr>::operator [] ("sys.symlink");
          (**buffer).truncateData(0);
          (**buffer).putData(linkpath, linksize + 1);
        }
      } else {
        diamond_static_err("msg=\"failed to read link\" path=%s errno=%d",
          lDirPath.c_str(), errno);
      }
    }
  }

  if (resync_listing) {
    diamond_static_info("resync no=%llx name=%s", dir.get()->getIno(),
      dir.get()->getName().c_str());
    diamond::common::RWMutexWriteLock dLock(dir.get()->Locker());
    struct dirent* dentry;

    // clean previous map/set
    dir.get()->getNamesInode().clear();
    dir.get()->std::set<diamond_ino_t>::clear();

    // that is root
    DIR* dp = opendir(lDirPath.c_str());

    while (dp && (dentry = readdir(dp))) {
      std::string lName = dentry->d_name;
      std::string lStatPath = lDirPath + "/" + lName;
      if ((lName == ".") || (lName == ".."))
        continue;

      if (!lstat(lStatPath.c_str(), &buf)) {
        diamond_static_info("child=%s ino=%u", dentry->d_name, buf.st_ino);
        diamond_ino_t inode = buf.st_ino;
        dir.get()->getNamesInode()[dentry->d_name] = inode;
        dir.get()->std::set<diamond_ino_t>::insert(inode);
        {
          diamond::common::RWMutexWriteLock iLock(mInodeMapMutex);

          if (buf.st_mode & S_IFREG)
            mFileInodeMap[inode] = lStatPath;
          if (buf.st_mode & S_IFDIR)
            mDirInodeMap[inode] = lStatPath;
          if (buf.st_mode & S_IFLNK)
            mFileInodeMap[inode] = lStatPath;
        }
      }
    }
    if (dp)
      closedir(dp);
    retc = syncMeta(dir.get(), lDirPath);
    update_stats = true;
  } else {
    dir.get()->Locker().LockRead();
    if (dir.get()->getSyncOps().size()) {
      dir.get()->Locker().UnLockRead();
      dir.get()->Locker().LockWrite();
      retc = syncMeta(dir.get(), lDirPath);
      dir.get()->Locker().UnLockWrite();
      update_stats = true;
    }
  }


  if (update_stats && !stat(lDirPath.c_str(), &buf)) {
    diamond::common::RWMutexWriteLock dLock(dir.get()->Locker());
    dir.get()->setStat(buf);
    if (resync_listing)
      dir.get()->setSyncTime(buf);
  }

  return retc;
}

int
PosixBackend::sync(diamondCache::diamondFilePtr& file)
{
  int rc = 0;
  diamond_static_info("ino=%llx name=%s",
    file.get()->getIno(),
    file.get()->getName().c_str()
    );

  diamond::common::RWMutexWriteLock dLock(file.get()->Locker());
  diamond::common::RWMutexReadLock iLock(mInodeMapMutex);

  std::string path = mFileInodeMap[file.get()->getIno()].c_str();
  diamond_static_info("path=%s", path.c_str());
  struct stat buf;

  // fetch the full meta-data if there is no op pending
  if ((!(file.get()->getSyncOps().size())) && !lstat(path.c_str(), &buf)) {
    diamond_static_info("path=%s action=full-stat-sync", path.c_str());
    file.get()->setStat(buf);
    if ((buf.st_mtim.tv_sec == file.get()->getSyncTime()->tv_sec) &&
      (buf.st_mtim.tv_nsec == file.get()->getSyncTime()->tv_nsec)) {
      diamond_static_info("file-fast-track:no-backend-changes ino=%llx name=%s", file.get()->getIno(),
        file.get()->getName().c_str());
      return rc;
    }
  }

  rc = syncMeta(file.get(), path.c_str());

  if (!lstat(path.c_str(), &buf)) {
    diamond_static_info("path=%s action=full-stat-sync %x", path.c_str(), buf.st_mode);
    file.get()->setStat(buf);
    file.get()->setSyncTime(buf);
    if (S_ISLNK(buf.st_mode)) {
      diamond_static_info("msg=\"reading link\" path=%s", path.c_str());
      char linkpath[4096];
      ssize_t linksize = 0;
      if ((linksize = readlink(path.c_str(), linkpath, sizeof(linkpath))) > 0) {
        {
          linkpath[linksize] = 0;
          diamond::common::BufferPtr buffer;
          buffer = file.get()->std::map<std::string, diamond::common::BufferPtr>::operator [] ("sys.symlink");
          (**buffer).truncateData(0);
          (**buffer).putData(linkpath, linksize + 1);
        }
      } else {
        diamond_static_err("msg=\"failed to read link\" path=%s errno=%d",
          path.c_str(), errno);
      }
    }
  }
  return rc;
}

int
PosixBackend::syncMeta(diamondMeta *meta, std::string path)
{
  int retc = 0;
  if (!meta->getSyncOps().size()) {
    // do a full fetch of the attribute map
    ssize_t size;
    char list[65536];
    size = listxattr(path.c_str(), list, 65536);
    std::map<std::string, std::string> lAttr;

    if (size > 0) {
      char* list_start = list;
      for (int i = 0; i < size; i++) {
        if (list[i])
          continue;
        std::string key = list_start;
        list_start = list + i + 1;

        {
          diamond::common::BufferPtr buffer;
          buffer = meta->std::map<std::string, diamond::common::BufferPtr>::operator [] (key);
          char value[65536];
          int ret_size = lgetxattr(path.c_str(), key.c_str(), value, 65536);
          if (ret_size > 0) {
            (**buffer).truncateData(0);
            (**buffer).putData(value, ret_size);
          }
        }
      }
    }
  } else {
    std::list<diamondMeta::action_t>::iterator it;
    it = meta->getSyncOps().begin();
    bool am_committed = false;
    while (it != meta->getSyncOps().end()) {
      fprintf(stderr, "action=%s\n", it->first.c_str());
      if (it->first == meta->kActionCommitXattr) {
        diamond::common::BufferPtr attr;
        attr = meta->std::map<std::string, diamond::common::BufferPtr>::operator [] (it->second);
        if (it->second == "sys.symlink") {
          // convert into a symlink
          if (unlink(path.c_str())) {
            diamond_static_err("msg=\"failed to unlink symlink placeholder\" path=%s errno=%d", path.c_str(), errno);
          } else {
            if (symlink(&(**attr)[0], path.c_str())) {
              diamond_static_err("msg=\"failed to symlink\" old-path=%s new-path=%s errno=%d", &(**attr)[0], path.c_str(), errno);
            }
          }
        }
        // commit kv attribute
        retc |= lsetxattr(path.c_str(), it->second.c_str(), &(**attr)[0], (*attr)->size(), 0);
      } else if (it->first == meta->kActionDeleteXattr) {
        // delete kv attribute
        retc |= lremovexattr(path.c_str(), it->second.c_str());
      } else if (it->first == meta->kActionFetchXattr) {
        // fetch kv attribute
        diamond::common::BufferPtr buffer;
        buffer = meta->std::map<std::string, diamond::common::BufferPtr>::operator [] (it->second);
        char value[65536];
        int ret_size = lgetxattr(path.c_str(), it->second.c_str(), value, 65536);
        if (ret_size > 0) {
          (**buffer).truncateData(0);
          (**buffer).putData(value, ret_size);
        } else {
          retc = -1;
        }
      } else if (it->first == meta->kActionCommitMode) {
        retc |= chmod(path.c_str(), meta->getStat()->st_mode);
      } else if (it->first == meta->kActionCommitUid) {
        retc |= chown(path.c_str(), meta->getStat()->st_uid, meta->getStat()->st_gid);
      } else if (it->first == meta->kActionCommitGid) {
        retc |= chown(path.c_str(), meta->getStat()->st_uid, meta->getStat()->st_gid);
      } else if (it->first == meta->kActionCommitSize) {
        retc |= truncate(path.c_str(), meta->getStat()->st_size);
      } else if (it->first == meta->kActionCommitRenamed) {
        retc |= rename(it->second.c_str(), path.c_str());
      } else
        if (
        (!am_committed) && (
        (it->first == meta->kActionCommitAMtime) ||
        (it->first == meta->kActionCommitAtime) ||
        (it->first == meta->kActionCommitMtime)
        )) {
        struct timespec times[2];
        times[0].tv_sec = meta->getStat()->st_atim.tv_sec;
        times[0].tv_nsec = meta->getStat()->st_atim.tv_nsec;
        times[1].tv_sec = meta->getStat()->st_mtim.tv_sec;
        times[1].tv_nsec = meta->getStat()->st_mtim.tv_nsec;
        retc |= utimensat(0, path.c_str(), times, 0);
        am_committed = true;
      } else if (it->first == meta->kActionCommitCtime) {
        // not implemented
      }
      std::list<diamondMeta::action_t>::iterator nit;
      nit = it;
      it++;
      meta->getSyncOps().erase(nit);
    }
  }
  return 0;
}

int
PosixBackend::mkFile(uid_t uid, gid_t gid, mode_t mode, diamondCache::diamondDirPtr parent, std::string& name, diamond_ino_t & new_inode)
{
  new_inode = 0;
  fprintf(stderr, "mkFile\n");
  diamond::common::RWMutexWriteLock iLock(mInodeMapMutex);
  if (!mDirInodeMap.count(parent.get()->getIno())) {
    fprintf(stderr, "ino=%llx\n", (unsigned long long) parent.get()->getIno());
    errno = ENOTDIR;
    return -1;
  }
  std::string path = mDirInodeMap[parent.get()->getIno()] + "/" + name;
  fprintf(stderr, "creat path=%s", path.c_str());
  setfsuid(uid);
  setfsgid(gid);
  int retc = mknod(path.c_str(), mode, S_IFREG);
  if (!retc) {
    // store new inode
    struct stat buf;
    if (!stat(path.c_str(), &buf)) {
      new_inode = buf.st_ino;
      mFileInodeMap[new_inode] = path;
    }
  }
  setfsuid(0);
  setfsgid(0);

  return retc;
}

int
PosixBackend::mkDir(uid_t uid,
  gid_t gid,
  mode_t mode,
  diamondCache::diamondDirPtr parent,
  std::string& name,
  diamond_ino_t & new_inode)
{
  new_inode = 0;
  fprintf(stderr, "mkDir\n");
  diamond::common::RWMutexWriteLock iLock(mInodeMapMutex);
  if (!mDirInodeMap.count(parent.get()->getIno())) {
    fprintf(stderr, "ino=%llx\n", (unsigned long long) parent.get()->getIno());
    errno = ENOTDIR;
    return -1;
  }
  std::string path = mDirInodeMap[parent.get()->getIno()] + "/" + name;
  fprintf(stderr, "mkdir path=%s", path.c_str());
  setfsuid(uid);
  setfsgid(gid);
  int retc = mkdir(path.c_str(), mode);
  if (!retc) {
    // store new inode
    struct stat buf;
    if (!stat(path.c_str(), &buf)) {
      new_inode = buf.st_ino;
      mDirInodeMap[new_inode] = path;
    }
  }
  setfsuid(0);
  setfsgid(0);

  return retc;
}

int
PosixBackend::rmFile(uid_t uid, gid_t gid, diamond_ino_t rm_inode)
{
  fprintf(stderr, "rmFile\n");
  diamond::common::RWMutexWriteLock iLock(mInodeMapMutex);

  if (!mFileInodeMap.count(rm_inode)) {
    fprintf(stderr, "ino=%llx\n", (unsigned long long) rm_inode);
    errno = ENOENT;
    return -1;
  }

  std::string path = mFileInodeMap[rm_inode];
  fprintf(stderr, "rmdir path=%s", path.c_str());
  setfsuid(uid);
  setfsgid(gid);
  int retc = unlink(path.c_str());
  setfsuid(0);
  setfsgid(0);
  if (!retc) {

    mFileInodeMap.erase(rm_inode);
  }
  return retc;
}

int
PosixBackend::rmDir(uid_t uid, gid_t gid, diamond_ino_t rm_inode)
{
  fprintf(stderr, "rmDir\n");
  diamond::common::RWMutexWriteLock iLock(mInodeMapMutex);

  if (!mDirInodeMap.count(rm_inode)) {
    fprintf(stderr, "ino=%llx\n", (unsigned long long) rm_inode);
    errno = ENOENT;
    return -1;
  }

  std::string path = mDirInodeMap[rm_inode];
  fprintf(stderr, "rmdir path=%s", path.c_str());
  setfsuid(uid);
  setfsgid(gid);
  int retc = rmdir(path.c_str());
  setfsuid(0);
  setfsgid(0);
  if (!retc) {
    mDirInodeMap.erase(rm_inode);
  }
  return retc;
}

int
PosixBackend::renameFile(uid_t uid,
  gid_t gid,
  diamond_ino_t rename_inode,
  diamond_ino_t old_parent_inode,
  diamond_ino_t new_parent_inode,
  std::string old_name,
  std::string new_name)
{
  fprintf(stderr, "renameFilen");

  if (!mDirInodeMap.count(old_parent_inode)) {
    fprintf(stderr, "ino=%llx\n", (unsigned long long) old_parent_inode);
    errno = ENOENT;
    return -1;
  }

  if (!mDirInodeMap.count(new_parent_inode)) {
    fprintf(stderr, "ino=%llx\n", (unsigned long long) old_parent_inode);
    errno = ENOENT;
    return -1;
  }

  if (!mFileInodeMap.count(rename_inode)) {
    fprintf(stderr, "ino=%llx\n", (unsigned long long) rename_inode);
    errno = ENOENT;
    return -1;
  }

  std::string old_path = mDirInodeMap[old_parent_inode] + "/" + old_name;
  std::string new_path = mDirInodeMap[new_parent_inode] + "/" + new_name;

  fprintf(stderr, "rename %s=>%s\n", old_path.c_str(), new_path.c_str());

  if (old_path == new_path)
    return 0;

  // rename in the backend
  int retc = rename(old_path.c_str(), new_path.c_str());

  if (retc) {
    return retc;
  }

  mFileInodeMap[rename_inode] = new_path;

  return 0;
}

int
PosixBackend::renameDir(uid_t uid,
  gid_t gid,
  diamond_ino_t rename_inode,
  diamond_ino_t old_parent_inode,
  diamond_ino_t new_parent_inode,
  std::string old_name,
  std::string new_name)
{
  fprintf(stderr, "renameDir\n");

  diamond::common::RWMutexWriteLock iLock(mInodeMapMutex);
  if (!mDirInodeMap.count(old_parent_inode)) {
    fprintf(stderr, "ino=%llx\n", (unsigned long long) old_parent_inode);
    errno = ENOENT;
    return -1;
  }

  if (!mDirInodeMap.count(new_parent_inode)) {
    fprintf(stderr, "ino=%llx\n", (unsigned long long) new_parent_inode);
    errno = ENOENT;
    return -1;
  }

  if (!mDirInodeMap.count(rename_inode)) {
    fprintf(stderr, "ino=%llx\n", (unsigned long long) rename_inode);
    errno = ENOENT;
    return -1;
  }

  std::string old_path = mDirInodeMap[old_parent_inode] + "/" + old_name;
  ;
  std::string new_path = mDirInodeMap[new_parent_inode] + "/" + new_name;

  if (old_path == new_path)
    return 0;

  fprintf(stderr, "rename %s=>%s\n", old_path.c_str(), new_path.c_str());

  // rename in the backend
  int retc = rename(old_path.c_str(), new_path.c_str());

  if (retc) {
    return retc;
  }

  std::map<diamond_ino_t, std::string>::const_iterator it;

  // loop over the whole dir map and replace
  for (it = mDirInodeMap.begin(); it != mDirInodeMap.end(); ++it) {
    if (it->second.substr(0, old_path.length()) == old_path) {
      std::string new_dir_path = it->second;
      new_dir_path.replace(0, old_path.length(), new_path);
      mDirInodeMap[it->first] = new_dir_path;
    }
  }

  // loop over the whole FILE map and replace
  for (it = mFileInodeMap.begin(); it != mFileInodeMap.end(); ++it) {
    if (it->second.substr(0, old_path.length()) == old_path) {
      std::string new_file_path = it->second;
      new_file_path.replace(0, old_path.length(), new_path);
      mFileInodeMap[it->first] = new_file_path;
    }
  }

  return 0;
}
DIAMONDRIONAMESPACE_END
