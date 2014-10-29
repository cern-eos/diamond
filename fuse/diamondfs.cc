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

static const char *os_str = "DiamondFS\n";

//static const char *os_name = "DiamondFS";

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


    diamondCache::diamondDirPtr inode = FS->getDir(DIAMOND_INODE(ino), false, false);

    struct stat* st = inode->getStat();
    diamond_static_debug("size=%d mode=%x ino=%llu inode=%llu (%d/%d) (%d/%d)", st->st_size, st->st_mode, ino, st->st_ino, sizeof(fuse_ino_t), sizeof(st->st_ino), sizeof(struct stat), sizeof(st));
    int rc = 0;
    if (!inode)
      fuse_reply_err(req, ENOENT);
    else 
      rc = fuse_reply_attr(req, st, attrcachetime);
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
      fuse_reply_err(req, ENOENT);
      return;
    }
    
    if (!inode->getNamesInode().count(name)) {
      fuse_reply_err(req, ENOENT);
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
	fuse_reply_err(req, ENOENT);
	return;
      }
      st = finode->getStat();
    } else {
      st = inode->getStat();
    }
    memcpy(&e.attr, st, sizeof(struct stat));
    dump_stat(&e.attr);
    fuse_reply_entry(req,&e);
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
      return fuse_reply_buf(req, buf + off, min(bufsize - off, maxsize));
    else
      return fuse_reply_buf(req, NULL, 0);
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
    diamond_static_debug("ino=%llu", ino);

    diamondCache::diamondDirPtr inode = FS->getDir(DIAMOND_INODE(ino), false, false);

    if (!inode) {
      diamondCache::diamondFilePtr finode = FS->getFile(DIAMOND_INODE(ino),false,false);
      if (!finode) {
	fuse_reply_err(req, ENOENT);
	return;
      } else {
	fuse_reply_err(req, ENOTDIR);
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
    fuse_reply_open(req, fi);
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
    fuse_reply_err(req, 0);
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

    fuse_reply_statfs(req, &stat_fs);
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
      fuse_reply_err(req, ENOENT);
      return ;
    }

    if (inode->getNamesInode().count(name)) {
      fuse_reply_err(req, EEXIST);
      return ;
    }

    // create a new entry
    diamond_ino_t new_ino = FS->newInode();
    diamondCache::diamondDirPtr new_inode = FS->getDir(new_ino, true, true, name );
    if (new_inode)
      new_inode->makeStat(fuse_req_ctx(req)->uid, fuse_req_ctx(req)->gid, DIAMOND_TO_INODE(new_ino), S_IFDIR | mode, 0);

    
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
    fuse_reply_entry(req, &e);
  }

  //--------------------------------------------------------------------------
  //! Remove (delete) the given file, symbolic link, hard link,or special node
  //--------------------------------------------------------------------------

  static void
  unlink (fuse_req_t req, fuse_ino_t parent, const char *name)
  {
    diamond_static_debug("");
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
      fuse_reply_err(req, ENOENT);
      return ;
    }

    if (!inode->getNamesInode().count(name)) {
      fuse_reply_err(req, ENOENT);
      return ;
    }

    diamond_ino_t ino = inode->getNamesInode()[name];
    diamondCache::diamondDirPtr child = FS->getDir(ino, false, false);

    if (!child) {
      fuse_reply_err(req, ENOENT);
      return ;
    }

    if (child->getNamesInode().size()) {
      fuse_reply_err(req, ENOTEMPTY);
      return ;
    }

    // remove 'name' directory
    if ( FS->rmDir(ino) ) {
      fuse_reply_err(req, ENOENT);
      return ;
    }

    // update parent
    inode->getNamesInode().erase(name);
    inode->std::set<diamond_ino_t>::erase(ino);
    
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
    diamond_static_debug("");
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
    diamond_static_debug("");
    /*
    if (ino != 2)
      fuse_reply_err(req, EISDIR);
    else if ((fi->flags & 3) != O_RDONLY)
      fuse_reply_err(req, EACCES);

    else
      fuse_reply_open(req, fi);
    */
    fuse_reply_err(req, ENOENT);
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
    (void) fi;
    diamond_static_debug("");

    assert(ino == 2);
    reply_buf_limited(req, os_str, strlen(os_str), off, size);
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
    fuse_reply_err(req, 0);
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
    diamond_static_debug("");
    fuse_reply_err(req, ENODATA);
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
    diamond_static_debug("");
  }

  //--------------------------------------------------------------------------
  //! List extended attributes
  //--------------------------------------------------------------------------

  static void
  listxattr (fuse_req_t req, fuse_ino_t ino, size_t size)
  {
    diamond_static_debug("");
  }

  //--------------------------------------------------------------------------
  //! Remove extended attribute
  //--------------------------------------------------------------------------

  static void
  removexattr (fuse_req_t req,
               fuse_ino_t ino,
               const char *xattr_name)
  {
    diamond_static_debug("");
  }

  //--------------------------------------------------------------------------
  //! Singleton
  //--------------------------------------------------------------------------
  static diamondfs* FS;

};

diamondfs* diamondfs::FS = 0;
double diamondfs::entrycachetime = 1.0;
double diamondfs::attrcachetime  = 1.0;


int
main (int argc, char *argv[])
{
  //----------------------------------------------------------------------------
  //! Runs the daemon at the mountpoint specified in argv and with other
  //! options if specified
  //----------------------------------------------------------------------------

  diamondfs fs;
  bool selftest = true;

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
	// create a new entry
	diamond_ino_t new_ino = fs.newInode();
	diamondCache::diamondDirPtr new_inode = fs.getDir(new_ino, true, true, std::to_string(i).c_str() );
	if (new_inode)
	new_inode->makeStat( i%10, i%10 , DIAMOND_TO_INODE(new_ino), S_IFDIR | S_IRWXU, 0);
	
	// attach to the parent
	selftest_inode->getNamesInode()[std::to_string(i)]=new_ino;
	selftest_inode->std::set<diamond_ino_t>::insert(new_ino);
      }
      COMMONTIMING("t1",&tm1);
    }

    {
      COMMONTIMING("t0",&tm2);
      for (auto it = selftest_inode->std::set<diamond_ino_t>::begin(); it != selftest_inode->std::set<diamond_ino_t>::end(); ++it) {
	diamondCache::diamondDirPtr inode = fs.getDir(*it, false, false);
      }
      COMMONTIMING("t1",&tm2);
    }

    {
      COMMONTIMING("t0",&tm3);
      for (auto it = selftest_inode->getNamesInode().begin(); it != selftest_inode->getNamesInode().end(); ++it) {
	selftest_inode->getNamesInode().erase(it->first);
	selftest_inode->std::set<diamond_ino_t>::erase(it->second);
	fs.rmDir(it->second);
      }
      COMMONTIMING("t1",&tm3);
    }
    {
      for (size_t i = 0; i < 100000; ++i) {
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


  if (selftest) {
    diamond_static_notice("unit=self-test create=%.02f kHz", 100000/tm1.RealTime());
    diamond_static_notice("unit=self-test lookup=%.02f kHz", 100000/tm2.RealTime());
    diamond_static_notice("unit=self-test remove=%.02f kHz", 100000/tm3.RealTime());
  }

  //----------------------------------------------------------------------------
  // start the FUSE daemon
  //----------------------------------------------------------------------------
  return fs.daemonize(argc, argv, &fs, NULL);
}
