#include "llfusexx.h"
#include <iostream>
#include <cstring>
#include <cerrno>
#include <cassert>
#include <cstdio>

#include "common/Logging.hh"
#include "common/Timing.hh"
#include "rio/diamondCache.hh"
#include <sys/statvfs.h>

struct statvfs stat_fs;

//------------------------------------------------------------------------------
//! Curiously recurring templates....
//!
//!   g++ -Wall `pkg-config fuse --cflags --libs` os.cpp -o os
//------------------------------------------------------------------------------

using namespace diamond::rio;

class diamondfs : public llfusexx::fs<diamondfs> , public diamond::rio::diamondCache
{
private:

public:

  static double entrycachetime; 
  static double attrcachetime;
  static bool fuse_do_reply;

  static void
  dump_stat(struct stat* st) 
  {
    diamond_static_debug("dev=%llx ino=%llx mode=%llx link=%llx uid=%llx gid=%llx rdev=%llx size=%llx blksize=%llx blocks=%llx atime=%llu mtime=%llu, ctime=%llu",
			 (unsigned long long) st->st_dev,
			 (unsigned long long) st->st_ino,
			 (unsigned long long) st->st_mode,
			 (unsigned long long) st->st_nlink,
			 (unsigned long long) st->st_uid,
			 (unsigned long long) st->st_gid, 
			 (unsigned long long) st->st_rdev,
			 (unsigned long long) st->st_size,
			 (unsigned long long) st->st_blksize,
			 (unsigned long long) st->st_blocks,
			 (unsigned long long) st->st_atime,
			 (unsigned long long) st->st_mtime,
			 (unsigned long long) st->st_ctime);
  }
  //--------------------------------------------------------------------------
  //! Constructor
  //--------------------------------------------------------------------------

  diamondfs () 
  { 
    diamond_static_debug("");
    FS = this;
  };

  //--------------------------------------------------------------------------
  //! Destructor
  //--------------------------------------------------------------------------

  virtual
  ~diamondfs () 
  { 
    diamond_static_debug("");
  };

  //--------------------------------------------------------------------------
  //! Initialize filesystem
  //--------------------------------------------------------------------------

  static void
  init (void *userdata, struct fuse_conn_info *conn)
  {
    diamond_static_debug("");
  }

  //--------------------------------------------------------------------------
  //! Clean up filesystem
  //--------------------------------------------------------------------------

  static void
  destroy (void *userdata)
  {
    diamond_static_debug("userdata=%llu",(unsigned long long)userdata);
  }

  //--------------------------------------------------------------------------
  //! Get file attributes
  //--------------------------------------------------------------------------

  static void
  getattr (fuse_req_t req,
           fuse_ino_t ino,
           struct fuse_file_info *fi)
  {
    diamond_static_debug("ino=%llx", ino);
    
    (void) fi;


    diamondCache::diamondFilePtr inode = FS->getFile(DIAMOND_INODE(ino), false, false);

    struct stat* st = 0;

    if (inode) {
      st = inode->getStat();
    } else {
      diamondCache::diamondDirPtr inode = FS->getDir(DIAMOND_INODE(ino), false, false);
      if (inode) {
	st = inode->getStat();
      } else {
	(!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
	return;
      }
    }

    diamond_static_debug("size=%d mode=%x ino=%llx inode=%llu (%d/%d) (%d/%d)", st->st_size, st->st_mode, ino, st->st_ino, sizeof(fuse_ino_t), sizeof(st->st_ino), sizeof(struct stat), sizeof(st));
    int rc = 0;
    rc = (!fuse_do_reply)?0:fuse_reply_attr(req, st, attrcachetime);
    diamond_static_debug("rc=%d", rc);
  }

  //--------------------------------------------------------------------------
  //! Change attributes of a file
  //--------------------------------------------------------------------------

  static void
  setattr (fuse_req_t req,
           fuse_ino_t ino,
           struct stat *attr,
           int to_set,
           struct fuse_file_info *fi)
  {
    diamond_static_debug("");

    diamondCache::diamondDirPtr dinode = FS->getDir(DIAMOND_INODE(ino), false, false);
    diamondFilePtr finode;

    struct stat* st = 0;

    if (!dinode) {
      finode = FS->getFile(DIAMOND_INODE(ino),false,false);
      if (!finode) {
	(!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
	return;
      } else {
	st = finode->getStat();
      }
    } else {
      st = dinode->getStat();
    }

    if (to_set & FUSE_SET_ATTR_MODE) {
      st->st_mode = attr->st_mode;
    }
    if (to_set & FUSE_SET_ATTR_UID) {
      st->st_uid = attr->st_uid;
    }
    if (to_set & FUSE_SET_ATTR_GID) {
      st->st_gid = attr->st_gid;
    }

    if (to_set & FUSE_SET_ATTR_SIZE) {
      if (attr->st_size < (1024ll*1024*1024*1024*16)) {
	st->st_size = attr->st_size;
	if (finode) {
	  finode->truncate(attr->st_size);
	}
      } else {
	(!fuse_do_reply)?0:fuse_reply_err(req, EFBIG);
      }
    }
    
    if (to_set & FUSE_SET_ATTR_ATIME) {
      st->st_atime = attr->st_atime;
      st->st_atim.tv_sec = attr->st_atim.tv_sec;
      st->st_atim.tv_nsec = attr->st_atim.tv_nsec;
    }

    if (to_set & FUSE_SET_ATTR_MTIME) {
      st->st_mtime = attr->st_mtime;
      st->st_mtim.tv_sec = attr->st_mtim.tv_sec;
      st->st_mtim.tv_nsec = attr->st_mtim.tv_nsec;
    }
    (!fuse_do_reply)?0:fuse_reply_attr (req, st, attrcachetime);
    return ;
  }

  //--------------------------------------------------------------------------
  //! Lookup an entry
  //--------------------------------------------------------------------------

  static void
  lookup (fuse_req_t req,
          fuse_ino_t parent,
          const char *name)
  {
    struct fuse_entry_param e;
    diamond_static_debug("name=%s", name);

    memset(&e, 0, sizeof ( e));

    diamondCache::diamondDirPtr inode = FS->getDir(DIAMOND_INODE(parent), false, false);

    if (!inode) {
      (!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
      return;
    }
    
    if (!inode->getNamesInode().count(name)) {
      (!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
      return;
    }
      
    e.ino = DIAMOND_TO_INODE(inode->getNamesInode()[name]);
    e.attr_timeout = attrcachetime;
    e.entry_timeout = entrycachetime;
    struct stat* st=0;
    inode = FS->getDir(DIAMOND_INODE(e.ino), false, false);
    if (!inode) {
      diamondCache::diamondFilePtr finode = FS->getFile(DIAMOND_INODE(e.ino), false, false);
      if (!finode) {
	// very unlikely if not impossible
	(!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
	return;
      }
      st = finode->getStat();
    } else {
      st = inode->getStat();
    }
    memcpy(&e.attr, st, sizeof(struct stat));
    dump_stat(&e.attr);
    (!fuse_do_reply)?0:fuse_reply_entry(req,&e);
  }

  struct dirbuf
  {
    char *p;
    size_t size;
    size_t alloc_size;
  };

  static void
  dirbuf_add (fuse_req_t req, struct dirbuf *b, const char *name,
              fuse_ino_t ino)
  {
    diamond_static_debug("name=%s ino=%llx", name, ino);

    struct stat stbuf;
    size_t oldsize = b->size;
    b->size += fuse_add_direntry(req, NULL, 0, name, NULL, 0);
    if (b->size > b->alloc_size) {
      // avoid reallocation for every single entry
      char *newp = (char*) realloc(b->p, b->size+(256*1024));
      if (!newp) {
	fprintf(stderr, "*** fatal error: cannot allocate memory\n");
	abort();
      }
      b->p = newp;
      b->alloc_size = b->size+(256*1024);
    }

    memset(&stbuf, 0, sizeof ( stbuf));
    stbuf.st_ino = ino;
    fuse_add_direntry(req, b->p + oldsize, b->size - oldsize, name, &stbuf,
                      b->size);
  }

#define min(x, y) ((x) < (y) ? (x) : (y))

  static int
  reply_buf_limited (fuse_req_t req, const char *buf,
                     size_t bufsize, off_t off, size_t maxsize)
  {
    if (off < (off_t)bufsize)
      return (!fuse_do_reply)?0:fuse_reply_buf(req, buf + off, min(bufsize - off, maxsize));
    else
      return (!fuse_do_reply)?0:fuse_reply_buf(req, NULL, 0);
  }

  //--------------------------------------------------------------------------
  //! Produce cached directory contents
  //--------------------------------------------------------------------------
  static void 
  opendir (fuse_req_t req, 
	   fuse_ino_t ino,
	   struct fuse_file_info *fi)
  {
    (void) fi;
    diamond_static_debug("ino=%llx", ino);

    diamondCache::diamondDirPtr inode = FS->getDir(DIAMOND_INODE(ino), false, false);

    if (!inode) {
      diamondCache::diamondFilePtr finode = FS->getFile(DIAMOND_INODE(ino),false,false);
      if (!finode) {
	(!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
	return;
      } else {
	(!fuse_do_reply)?0:fuse_reply_err(req, ENOTDIR);
	return;
      }
    }
    
    fi->fh = (uint64_t) new struct dirbuf;
    struct dirbuf* b = (struct dirbuf*) fi->fh;

    memset(b, 0, sizeof ( struct dirbuf));

    dirbuf_add(req, b, ".", ino);
    dirbuf_add(req, b, "..", ino);
    
    for (std::set<diamond_ino_t>::const_iterator it = (*inode).std::set<diamond_ino_t>::begin(); it != (*inode).std::set<diamond_ino_t>::end(); ++it) {
      // get each inode to add the name
      diamondCache::diamondDirPtr dinode = FS->getDir(*it,false,false);
      if (!dinode) {
	diamondCache::diamondFilePtr finode = FS->getFile(*it,false,false);
	if (finode) {
	  dirbuf_add(req, b, finode->getName().c_str(), DIAMOND_TO_INODE(*it));
	}
      } else {
	dirbuf_add(req, b, dinode->getName().c_str(), DIAMOND_TO_INODE(*it));
      }
    }
    (!fuse_do_reply)?0:fuse_reply_open(req, fi);
  }

  //--------------------------------------------------------------------------
  //! Read the entries from a directory
  //--------------------------------------------------------------------------

  static void
  readdir (fuse_req_t req,
           fuse_ino_t ino,
           size_t size,
           off_t off,
           struct fuse_file_info *fi)
  {
    if (fi && fi->fh) {
      struct dirbuf* b = (struct dirbuf*) fi->fh;
      reply_buf_limited(req, b->p, b->size, off, size);
    }
  }

  //--------------------------------------------------------------------------
  //! Release cached directory contents
  //--------------------------------------------------------------------------

  static void
  releasedir (fuse_req_t req, 
	      fuse_ino_t ino,
	      struct fuse_file_info *fi)
  {
    if (fi->fh) {
      struct dirbuf* b = (struct dirbuf*) fi->fh;

      free(b->p);
      delete b;
      fi->fh = 0;
    }
    (!fuse_do_reply)?0:fuse_reply_err(req, 0);
  }

  //--------------------------------------------------------------------------
  //! Return statistics about the filesystem
  //--------------------------------------------------------------------------

  static void
  statfs (fuse_req_t req, fuse_ino_t ino)
  {
    diamond_static_debug("");
    memset(&stat_fs, 0, sizeof (stat_fs));

    stat_fs.f_bsize = 4096;
    stat_fs.f_frsize = 4096;
    stat_fs.f_blocks = 1000000ll;
    stat_fs.f_bfree = 1000000ll;
    stat_fs.f_bavail = 1000000ll;
    stat_fs.f_files = 1000000ll;
    stat_fs.f_ffree = 1000000ll;
    stat_fs.f_fsid = 99;
    stat_fs.f_flag = 1;
    stat_fs.f_namemax = 512;

    (!fuse_do_reply)?0:fuse_reply_statfs(req, &stat_fs);
  }

  //--------------------------------------------------------------------------
  //! Make a special (device) file, FIFO, or socket
  //--------------------------------------------------------------------------

  static void
  mknod (fuse_req_t req,
         fuse_ino_t parent,
         const char *name,
         mode_t mode,
         dev_t rdev)
  {
    diamond_static_debug("");
  }

  //--------------------------------------------------------------------------
  //! Create a directory with a given name
  //--------------------------------------------------------------------------

  static void
  mkdir (fuse_req_t req,
         fuse_ino_t parent,
         const char *name,
         mode_t mode)
  {
    diamond_static_debug("");

    diamondCache::diamondDirPtr inode = FS->getDir(DIAMOND_INODE(parent), false, false);

    if (!inode) {
      (!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
      return ;
    }

    if (inode->getNamesInode().count(name)) {
      (!fuse_do_reply)?0:fuse_reply_err(req, EEXIST);
      return ;
    }

    // create a new entry
    diamond_ino_t new_ino = FS->newInode();
    diamondCache::diamondDirPtr new_inode = FS->getDir(new_ino, true, true, name );
    if (new_inode)
      new_inode->makeStat(req?(fuse_req_ctx(req)->uid):0, req?(fuse_req_ctx(req)->gid):0, DIAMOND_TO_INODE(new_ino), S_IFDIR | mode, 0);

    
    struct fuse_entry_param e;
    memcpy(&e.attr, new_inode->getStat(), sizeof(struct stat));

    e.ino = e.attr.st_ino;
    e.attr_timeout  = attrcachetime;
    e.entry_timeout = entrycachetime;

    // attach to the parent
    inode->getNamesInode()[name]=new_ino;
    inode->std::set<diamond_ino_t>::insert(new_ino);
    dump_stat(&e.attr);
    diamond_static_debug("ino=%s name=%s\n", new_inode->getIno().c_str(), new_inode->getName().c_str());
    (!fuse_do_reply)?0:fuse_reply_entry(req, &e);
  }

  //--------------------------------------------------------------------------
  //! Remove (delete) the given file, symbolic link, hard link,or special node
  //--------------------------------------------------------------------------

  static void
  unlink (fuse_req_t req, fuse_ino_t parent, const char *name)
  {
    diamond_static_debug("");

    diamondCache::diamondDirPtr inode = FS->getDir(DIAMOND_INODE(parent), false, false);

    if (!inode) {
      (!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
      return ;
    }

    if (!inode->getNamesInode().count(name)) {
      (!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
      return ;
    }

    diamond_ino_t ino = inode->getNamesInode()[name];
    diamondCache::diamondFilePtr child = FS->getFile(ino, false, false);

    if (!child) {
      (!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
      return ;
    }

    // remove 'name' directory
    if ( FS->rmFile(ino) ) {
      (!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
      return ;
    }

    // update parent
    inode->getNamesInode().erase(name);
    inode->std::set<diamond_ino_t>::erase(ino);
    (!fuse_do_reply)?0:fuse_reply_err(req,0);
    return;
  }

  //--------------------------------------------------------------------------
  //! Remove the given directory
  //--------------------------------------------------------------------------

  static void
  rmdir (fuse_req_t req, fuse_ino_t parent, const char *name)
  {
    diamond_static_debug("");

    diamondCache::diamondDirPtr inode = FS->getDir(DIAMOND_INODE(parent), false, false);

    if (!inode) {
      (!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
      return ;
    }

    if (!inode->getNamesInode().count(name)) {
      (!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
      return ;
    }

    diamond_ino_t ino = inode->getNamesInode()[name];
    diamondCache::diamondDirPtr child = FS->getDir(ino, false, false);

    if (!child) {
      (!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
      return ;
    }

    if (child->getNamesInode().size()) {
      (!fuse_do_reply)?0:fuse_reply_err(req, ENOTEMPTY);
      return ;
    }

    // remove 'name' directory
    if ( FS->rmDir(ino) ) {
      (!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
      return ;
    }

    // update parent
    inode->getNamesInode().erase(name);
    inode->std::set<diamond_ino_t>::erase(ino);

    (!fuse_do_reply)?0:fuse_reply_err(req,0);    
    return;
  }

  //--------------------------------------------------------------------------
  //! Rename the file, directory, or other object
  //--------------------------------------------------------------------------

  static void
  rename (fuse_req_t req,
          fuse_ino_t parent,
          const char *name,
          fuse_ino_t newparent,
          const char *newname)
  {
    diamond_static_debug("parent=%llx newparent=%llx name=%s newname=%s", parent, newparent, name, newname);

    diamondCache::diamondDirPtr dinode;
    diamondCache::diamondDirPtr tinode;

    dinode = FS->getDir(DIAMOND_INODE(parent), false, false);
    tinode = FS->getDir(DIAMOND_INODE(newparent), false, false);
    
    if ( (!dinode) || (!tinode)) {
      (!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
      return ;
    }

    if (!dinode->getNamesInode().count(name)) {
      (!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
      return ;
    }

    diamond_ino_t ino = dinode->getNamesInode()[name];

    if (tinode->getNamesInode().count(newname)) {
      diamond_ino_t tino = tinode->getNamesInode()[newname];
    
      //the target exists
      if (FS->rmFile(tino))
	FS->rmDir(tino);
      tinode->getNamesInode().erase(newname);
      tinode->std::set<diamond_ino_t>::erase(tino);
    }
      
    // update source
    dinode->getNamesInode().erase(name);
    dinode->std::set<diamond_ino_t>::erase(ino);

    // update target
    tinode->getNamesInode()[newname]=ino;
    tinode->std::set<diamond_ino_t>::insert(ino);
    
    // rename the object itself

    diamondCache::diamondFilePtr finode = FS->getFile(ino,false,false);
    if (!finode) {
      diamondCache::diamondDirPtr dinode = FS->getDir(ino, false, false);
      if (!dinode) {    
	// uups
      } else {
	dinode->setName(newname);
      }
    } else {
      finode->setName(newname);
    }

    (!fuse_do_reply)?0:fuse_reply_err(req,0);
    return;
  }

  //--------------------------------------------------------------------------
  //
  //--------------------------------------------------------------------------

  static void
  access (fuse_req_t req, fuse_ino_t ino, int mask)
  {
    diamond_static_debug("");
  }

  //--------------------------------------------------------------------------
  //! Open a file
  //--------------------------------------------------------------------------

  static void
  open (fuse_req_t req,
        fuse_ino_t ino,
        struct fuse_file_info * fi)
  {
    diamond_static_debug("ino=%llx", ino);
    
    diamondCache::diamondFilePtr* fptr = new diamondCache::diamondFilePtr;
    *fptr = FS->getFile(DIAMOND_INODE(ino), true, false);
    
    if (!fptr) {
      (!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
      return;
    }
    // store the shared pointer the file as file handle
    fi->fh = (uint64_t) fptr;
    //    fi->direct_io = 1;
    (!fuse_do_reply)?0:fuse_reply_open(req, fi);
    return;
  }

  //--------------------------------------------------------------------------
  //! Create a file
  //--------------------------------------------------------------------------
  static void 
  create (fuse_req_t req, 
	  fuse_ino_t parent, 
	  const char *name,
	  mode_t mode, 
	  struct fuse_file_info *fi)
  {
    diamondCache::diamondFilePtr* fptr = new diamondCache::diamondFilePtr;

    diamondCache::diamondDirPtr inode = FS->getDir(DIAMOND_INODE(parent), false, false);

    if (!inode) {
      (!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
      return;
    }
    
    if (inode->getNamesInode().count(name)) {
      (!fuse_do_reply)?0:fuse_reply_err(req, EEXIST);
      return;
    }

    diamond_ino_t ino = FS->newInode();
    *fptr = FS->getFile(ino, true, true, name);
    
    if (!fptr) {
      (!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
      return;
    }
    // store the shared pointer the file as file handle
    fi->fh = (uint64_t) fptr;

    if (*fptr)
      (*fptr)->makeStat(req?(fuse_req_ctx(req)->uid):0, req?(fuse_req_ctx(req)->gid):0, DIAMOND_TO_INODE(ino) , S_IFREG | mode, 0);
    
    struct fuse_entry_param e;
    memcpy(&e.attr, (*fptr)->getStat(), sizeof(struct stat));

    e.ino = e.attr.st_ino;
    e.attr_timeout  = attrcachetime;
    e.entry_timeout = entrycachetime;

    // attach to the parent
    inode->getNamesInode()[name]=ino;
    inode->std::set<diamond_ino_t>::insert(ino);
    dump_stat(&e.attr);
    diamond_static_debug("ino=%s name=%s\n", (*fptr)->getIno().c_str(), (*fptr)->getName().c_str());
    (!fuse_do_reply)?0:fuse_reply_create(req, &e, fi);
  }

  //--------------------------------------------------------------------------
  //! Read from file. Returns the number of bytes transferred, or 0 if offset
  //! was at or beyond the end of the file.
  //--------------------------------------------------------------------------
  
  static void
  read (fuse_req_t req,
        fuse_ino_t ino,
        size_t size,
        off_t off,
        struct fuse_file_info * fi)
  {
    diamondCache::diamondFilePtr* file = ((diamondCache::diamondFilePtr*)fi->fh);
    
    char* buffer=0;
    size_t s = (*file)->peek(buffer, off, size);
    diamond_static_debug("ino=%llx off=%llx size=%llu avail=%u", (unsigned long long)ino, (unsigned long long)off, (unsigned long long)size, s);
    
    (!fuse_do_reply)?0:fuse_reply_buf(req, buffer, s);
    (*file)->release();
    return;
  }

  //--------------------------------------------------------------------------
  //! Write function
  //--------------------------------------------------------------------------

  static void
  write (fuse_req_t req,
         fuse_ino_t ino,
         const char *buf,
         size_t size,
         off_t off,
         struct fuse_file_info * fi)
  {
    diamond_static_debug("");
    diamondCache::diamondFilePtr* file = ((diamondCache::diamondFilePtr*)fi->fh);
    diamond_static_debug("ino=%llx off=%llx size=%llu", (unsigned long long)ino, (unsigned long long)off, (unsigned long long)size);
    off_t s = (*file)->write(buf, off, size);
    diamond_static_debug("size=%u offset=%llu", size, s);
    (!fuse_do_reply)?0:fuse_reply_write(req, size);
    return;
  }

  //--------------------------------------------------------------------------
  //! Release is called when FUSE is completely done with a file; at that point,
  //! you can free up any temporarily allocated data structures.
  //--------------------------------------------------------------------------

  static void
  release (fuse_req_t req,
           fuse_ino_t ino,
           struct fuse_file_info * fi)
  {
    diamond_static_debug("");
    if (fi && fi->fh) {
      if (fi->fh) delete ((diamondCache::diamondFilePtr*)fi->fh);
    }
    (!fuse_do_reply)?0:fuse_reply_err(req, 0);
  }

  //--------------------------------------------------------------------------
  //! Flush any dirty information about the file to disk
  //--------------------------------------------------------------------------

  static void
  fsync (fuse_req_t req,
         fuse_ino_t ino,
         int datasync,
         struct fuse_file_info * fi)
  {
   diamond_static_debug("");
  }

  //--------------------------------------------------------------------------
  //! Forget inode <-> path mapping
  //--------------------------------------------------------------------------

  static void
  forget (fuse_req_t req, fuse_ino_t ino, unsigned long nlookup)
  {
    diamond_static_debug("");
    (!fuse_do_reply)?0:fuse_reply_err(req, 0);
    return;
  }

  //--------------------------------------------------------------------------
  //! Called on each close so that the filesystem has a chance to report delayed errors
  //! Important: there may be more than one flush call for each open.
  //! Note: There is no guarantee that flush will ever be called at all!
  //--------------------------------------------------------------------------

  static void
  flush (fuse_req_t req,
         fuse_ino_t ino,
         struct fuse_file_info * fi)
  {
    diamond_static_debug("");
    (!fuse_do_reply)?0:fuse_reply_err(req, 0);
    return;
  }

  //--------------------------------------------------------------------------
  //! Get an extended attribute
  //--------------------------------------------------------------------------
#ifdef __APPLE__
  static void getxattr (fuse_req_t req,
                        fuse_ino_t ino,
                        const char *name,
                        size_t size,
                        uint32_t position)
#else

  static void
  getxattr (fuse_req_t req,
            fuse_ino_t ino,
            const char *name,
            size_t size)
#endif
  {
    diamond_static_debug("name=%s size=%u", name, size);

    // ignore capability requests
    if (std::string(name) == "security.capability") {
      (!fuse_do_reply)?0:fuse_reply_err(req, ENODATA);
      return;
    }

    diamond::common::BufferPtr attr;

    // try to get a file or a directory with that inode
    diamondCache::diamondFilePtr finode = FS->getFile(DIAMOND_INODE(ino),false,false);
    if (!finode) {
      diamondCache::diamondDirPtr dinode = FS->getDir(DIAMOND_INODE(ino), false, false);
      if (!dinode) {
	(!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
	return;
      }
      attr = dinode->std::map<std::string, diamond::common::BufferPtr>::operator [] (name);
    } else {
      attr = finode->std::map<std::string, diamond::common::BufferPtr>::operator [] (name);
    }

    if (size == 0) {
      (!fuse_do_reply)?0:fuse_reply_xattr(req, (*attr)->size());
      return ;
    }


    if ((*attr)->size() > size)
      	(!fuse_do_reply)?0:fuse_reply_err(req, ERANGE);
    else
      (!fuse_do_reply)?0:fuse_reply_buf(req, &(**attr)[0], (*attr)->size());
    return;
  }

  //--------------------------------------------------------------------------
  //! Set extended attribute
  //--------------------------------------------------------------------------
#ifdef __APPLE__
  static void setxattr (fuse_req_t req,
                        fuse_ino_t ino,
                        const char *name,
                        const char *value,
                        size_t size,
                        int flags,
                        uint32_t position)
#else

  static void
  setxattr (fuse_req_t req,
            fuse_ino_t ino,
            const char *name,
            const char *value,
            size_t size,
            int flags)
#endif
  {
    diamond_static_debug("name=%s size=%d", name, size);
    diamond::common::BufferPtr buffer;

    diamondCache::diamondFilePtr finode = FS->getFile(DIAMOND_INODE(ino),false,false);
    if (!finode) {
      diamondCache::diamondDirPtr dinode = FS->getDir(DIAMOND_INODE(ino), false, false);
      if (!dinode) {
	(!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
	return;
      }
      if (flags && dinode->std::map<std::string, diamond::common::BufferPtr>::count(name)) {
	(!fuse_do_reply)?0:fuse_reply_err(req, EEXIST);
	return;
      }
      buffer = dinode->std::map<std::string, diamond::common::BufferPtr>::operator [] (name);
    } else {
      if (flags && finode->std::map<std::string, diamond::common::BufferPtr>::count(name)) {
	(!fuse_do_reply)?0:fuse_reply_err(req, EEXIST);
	return;
      }
      buffer = finode->std::map<std::string, diamond::common::BufferPtr>::operator [] (name);     
    }
    (**buffer).putData(value,size);
    (!fuse_do_reply)?0:fuse_reply_err(req,0);
    return;
  }

  //--------------------------------------------------------------------------
  //! List extended attributes
  //--------------------------------------------------------------------------

  static void
  listxattr (fuse_req_t req, fuse_ino_t ino, size_t size)
  {
    diamond_static_debug("");

    std::map<std::string, diamond::common::BufferPtr>::const_iterator it;
    std::map<std::string, diamond::common::BufferPtr>::const_iterator it_end;

    // try to get a file or a directory with that inode
    diamondCache::diamondFilePtr finode = FS->getFile(DIAMOND_INODE(ino),false,false);
    if (!finode) {
      diamondCache::diamondDirPtr dinode = FS->getDir(DIAMOND_INODE(ino), false, false);
      if (!dinode) {
	(!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
	return;
      }
      it = dinode->std::map<std::string, diamond::common::BufferPtr>::begin();
      it_end = dinode->std::map<std::string, diamond::common::BufferPtr>::end();
    } else {
      it = finode->std::map<std::string, diamond::common::BufferPtr>::begin();
      it_end = finode->std::map<std::string, diamond::common::BufferPtr>::end();
    }
    
    diamond::common::BufferPtr buffer;
    for ( ; it != it_end; ++it) {
      (**buffer).putData(it->first.c_str(), it->first.length()+1);
    }

    if (size == 0) {
      (!fuse_do_reply)?0:fuse_reply_xattr(req, (**buffer).size());
      return ;
    }

    if ((**buffer).size() > size)
      	(!fuse_do_reply)?0:fuse_reply_err(req, ERANGE);
    else
      (!fuse_do_reply)?0:fuse_reply_buf(req, &(**buffer)[0], (**buffer).size());
    return ;
  }

  //--------------------------------------------------------------------------
  //! Remove extended attribute
  //--------------------------------------------------------------------------

  static void
  removexattr (fuse_req_t req,
               fuse_ino_t ino,
               const char *name)
  {
    diamond_static_debug("");
    diamond::common::BufferPtr attr;

    int items_removed=0;

    // try to get a file or a directory with that inode
    diamondCache::diamondFilePtr finode = FS->getFile(DIAMOND_INODE(ino),false,false);
    if (!finode) {
      diamondCache::diamondDirPtr dinode = FS->getDir(DIAMOND_INODE(ino), false, false);
      if (!dinode) {
	(!fuse_do_reply)?0:fuse_reply_err(req, ENOENT);
	return;
      }
      items_removed = dinode->std::map<std::string, diamond::common::BufferPtr>::erase(name);
    } else {
      items_removed = finode->std::map<std::string, diamond::common::BufferPtr>::erase(name);
    }

    if (!items_removed)
      (!fuse_do_reply)?0:fuse_reply_err(req, ENODATA);
    else
      (!fuse_do_reply)?0:fuse_reply_err(req, 0);
    return;
  }

  //--------------------------------------------------------------------------
  //! Singleton
  //--------------------------------------------------------------------------
  static diamondfs* FS;

};

diamondfs* diamondfs::FS = 0;
double diamondfs::entrycachetime = 1.0;
double diamondfs::attrcachetime  = 1.0;
bool diamondfs::fuse_do_reply=1;

int
main (int argc, char *argv[])
{
  //----------------------------------------------------------------------------
  //! Runs the daemon at the mountpoint specified in argv and with other
  //! options if specified
  //----------------------------------------------------------------------------

  diamondfs fs;
  bool selftest = false;

  //----------------------------------------------------------------------------
  // Configure the Logging
  // Two Env vars define the log leve:
  // export DIAMONDFS_FUSE_DEBUG=1 to run in debug mode
  // export DIAMONDFS_FUSE_LOGLEVEL=<n> to set the log leve different from LOG_INFO
  //----------------------------------------------------------------------------
  diamond::common::Logging::Init();
  diamond::common::Logging::SetUnit("FUSE/DiamondFS");
  diamond::common::Logging::gShortFormat = true;
  std::string fusedebug = getenv("DIAMONDFS_FUSE_DEBUG")?getenv("DIAMONDFS_FUSE_DEBUG"):"0";
  
  if ((getenv("DIAMONDFS_FUSE_DEBUG")) && (fusedebug != "0"))
  {
    diamond::common::Logging::SetLogPriority(LOG_DEBUG);
  }
  else
  { 
    if ((getenv("DIAMONDFS_FUSE_LOGLEVEL"))) 
    {
      diamond::common::Logging::SetLogPriority(atoi(getenv("DIAMONDFS_FUSE_LOGLEVEL")));
    } 
    else 
    {
      diamond::common::Logging::SetLogPriority(LOG_INFO);
    }
  }

  // create root node
  diamond_ino_t root_ino = fs.newInode();
  diamondCache::diamondDirPtr root = fs.getDir(root_ino, true, true, "/");
  if (root)
    root->makeStat(0,0,0, S_IFDIR | 0777, 1);

  std::stringstream s;
  fs.DumpCachedDirs(s);
  std::cerr << s.str();

  diamond::common::Timing tm1("mkdir");
  diamond::common::Timing tm2("stat");
  diamond::common::Timing tm3("rmdir");
  diamond::common::Timing tm4("create");

  // disable fuse replying
  diamondfs::fuse_do_reply=0;
  if (selftest) {
    diamond_ino_t selftest_ino;
    diamondCache::diamondDirPtr selftest_inode;

    {
      COMMONTIMING("t0",&tm1);
      //----------------------------------------------------------------------------
      // self test
      //----------------------------------------------------------------------------
      selftest_ino = fs.newInode();
      selftest_inode = fs.getDir(selftest_ino, true, true, ".selftest" );
      if (selftest_inode)
	selftest_inode->makeStat(0,0,DIAMOND_TO_INODE(selftest_ino), S_IFDIR | 0777, 0);
      
      root->getNamesInode()[".selftest"]=selftest_ino;
      root->std::set<diamond_ino_t>::insert(selftest_ino);

      for (size_t i = 0; i < 100000; ++i) {
	fs.mkdir ((fuse_req_t)0, DIAMOND_TO_INODE(selftest_ino), std::to_string(i).c_str(), S_IRWXU);
      }
      COMMONTIMING("t1",&tm1);
    }

    {
      COMMONTIMING("t0",&tm2);
      for (size_t i=0; i < 100000; ++i) {
	fs.lookup((fuse_req_t) 0, DIAMOND_TO_INODE(selftest_ino), std::to_string(i).c_str());
      }
      /*      for (auto it = selftest_inode->std::set<diamond_ino_t>::begin(); it != selftest_inode->std::set<diamond_ino_t>::end(); ++it) {
	diamondCache::diamondDirPtr inode = fs.getDir(*it, false, false);
	}*/
      COMMONTIMING("t1",&tm2);
    }

    {
      COMMONTIMING("t0",&tm3);
      for (size_t i=0; i< 100000; ++i) {
	fs.rmdir ((fuse_req_t) 0, DIAMOND_TO_INODE(selftest_ino), std::to_string(i).c_str());
      }
      COMMONTIMING("t1",&tm3);
    }
    {
      COMMONTIMING("t0",&tm4);
      for (size_t i=0 ; i< 1000; ++i) {
	struct fuse_file_info fi;
	fs.create ((fuse_req_t)0, DIAMOND_TO_INODE(selftest_ino), (std::string("f") + std::to_string(i)).c_str(), S_IRWXU, &fi);
	if (fi.fh) delete ((diamondCache::diamondFilePtr*)fi.fh);
      }
      COMMONTIMING("t1",&tm4);
    }
    {
      for (size_t i = 0; i < 10000; ++i) {
	// create a new entry
	diamond_ino_t new_ino = fs.newInode();
	diamondCache::diamondDirPtr new_inode = fs.getDir(new_ino, true, true, std::to_string(i).c_str() );
	if (new_inode)
	  new_inode->makeStat( i%10, i%10 , DIAMOND_TO_INODE(new_ino), S_IFDIR | S_IRWXU, 0);
	
	// attach to the parent
	selftest_inode->getNamesInode()[std::to_string(i)]=new_ino;
	selftest_inode->std::set<diamond_ino_t>::insert(new_ino);
      }
    }
  }

  // enable fuse replying
  diamondfs::fuse_do_reply=1;
  if (selftest) {
    diamond_static_notice("unit=self-test create=%.02f kHz", 100000/tm1.RealTime());
    diamond_static_notice("unit=self-test lookup=%.02f kHz", 100000/tm2.RealTime());
    diamond_static_notice("unit=self-test remove=%.02f kHz", 100000/tm3.RealTime());
    diamond_static_notice("unit=self-test create=%.02f kHz", 10000/tm4.RealTime());
  }

  //----------------------------------------------------------------------------
  // start the FUSE daemon
  //----------------------------------------------------------------------------
  return fs.daemonize(argc, argv, &fs, NULL);
}
