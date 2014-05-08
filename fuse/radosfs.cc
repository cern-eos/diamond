#include "llfusexx.h"
#include <iostream>
#include <cstring>
#include <cerrno>
#include <cassert>
#include <cstdio>

#include "common/Logging.hh"

static const char *os_str = "RadosFS\n";
//static const char *os_name = "RadosFS";

struct statvfs stat_fs;

//------------------------------------------------------------------------------
//! Curiously recurring templates....
//!
//!   g++ -Wall `pkg-config fuse --cflags --libs` os.cpp -o os
//------------------------------------------------------------------------------

class radosfs : public llfusexx::fs<radosfs> 
{
private:

public:
  //--------------------------------------------------------------------------
  //! Constructor
  //--------------------------------------------------------------------------

  radosfs () 
  { 
    diamond_static_debug("");
  };

  //--------------------------------------------------------------------------
  //! Destructor
  //--------------------------------------------------------------------------

  virtual
  ~radosfs () 
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
    struct stat stbuf;
    diamond_static_debug("");
    
    (void) fi;

    memset(&stbuf, 0, sizeof ( stbuf));

    /*
    int rc = sDirSvc.Stat(sNameSvc.GetPath(ino), stbuf);
    if (rc)
      fuse_reply_err(req, rc);
    else
      fuse_reply_attr(req, &stbuf, 1.0);
    */
    fuse_reply_err(req, ENOENT);
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
    diamond_static_debug("");

    printf("%lu %s\n", parent, name);
    memset(&e, 0, sizeof ( e));

    //    struct stat stbuf;
    
    /*
    e.ino = sNameSvc.GetInode(parent, name);
    e.attr_timeout = 1.0;
    e.entry_timeout = 1.0;
    int rc = sDirSvc.Stat(sNameSvc.GetPath(e.ino), stbuf);
    if (rc)
      fuse_reply_err(req, rc);
    else
      fuse_reply_entry(req, &e);
    */
    
  }

  struct dirbuf
  {
    char *p;
    size_t size;
  };

  static void
  dirbuf_add (fuse_req_t req, struct dirbuf *b, const char *name,
              fuse_ino_t ino)
  {
    diamond_static_debug("");

    struct stat stbuf;
    size_t oldsize = b->size;
    b->size += fuse_add_direntry(req, NULL, 0, name, NULL, 0);
    char *newp = (char*) realloc(b->p, b->size);
    if (!newp)
    {
      fprintf(stderr, "*** fatal error: cannot allocate memory\n");
      abort();
    }
    b->p = newp;
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
    /*
    if (off < (off_t)bufsize)
      return fuse_reply_buf(req, buf + off, min(bufsize - off, maxsize));
    else
      return fuse_reply_buf(req, NULL, 0);
    */
    return -1;
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
    (void) fi;
    diamond_static_debug("");

    /*
    if (ino != 1)
      fuse_reply_err(req, ENOTDIR);
    else
    {
      struct dirbuf b;
      DirectoryServer::dir_list_t lDirList;

      int rc = sDirSvc.List(sNameSvc.GetPath(ino), lDirList);
      if (rc)
      {
        fuse_reply_err(req, rc);
      }
      else
      {
        memset(&b, 0, sizeof ( b));

        for (auto it = lDirList.begin(); it != lDirList.end(); ++it)
        {
          dirbuf_add(req, &b, it->first.c_str(), it->second);
        }

        reply_buf_limited(req, b.p, b.size, off, size);
        free(b.p);
      }
    }
    */
    fuse_reply_err(req, ENOTDIR);
  }

  //--------------------------------------------------------------------------
  //! Return statistics about the filesystem
  //--------------------------------------------------------------------------

  static void
  statfs (fuse_req_t req, fuse_ino_t ino)
  {
    diamond_static_debug("");
    memset(&stat_fs, 0, sizeof (stat_fs));

    stat_fs.f_bsize = 65536;
    stat_fs.f_blocks = 1000000ll;
    stat_fs.f_bfree = 1000000ll;
    stat_fs.f_bavail = 1000000ll;
    stat_fs.f_files = 1000000ll;
    stat_fs.f_ffree = 1000000ll;
    stat_fs.f_fsid = 99;

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
    /*
    fuse_ino_t ino = sNameSvc.GetInode(parent, name);
    int rc = sDirSvc.Mkdir(sNameSvc.GetPath(ino),
                           ino,
                           mode);
    fuse_reply_err(req, rc);
    */
    fuse_reply_err(req, ENOENT);
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
    /*
    fuse_ino_t ino = sNameSvc.GetInode(parent, name);
    int rc = sDirSvc.Rmdir(sNameSvc.GetPath(ino), ino);
    if (!rc)
    {

      sNameSvc.ForgetInode(ino, parent, name);
    }
    fuse_reply_err(req, rc);
    */
    fuse_reply_err(req, ENOENT);
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
};

int
main (int argc, char *argv[])
{
  //----------------------------------------------------------------------------
  //! Runs the daemon at the mountpoint specified in argv and with other
  //! options if specified
  //----------------------------------------------------------------------------

  radosfs fs;

  //----------------------------------------------------------------------------
  // Configure the Logging
  // Two Env vars define the log leve:
  // export RADOSFS_FUSE_DEBUG=1 to run in debug mode
  // export RADOSFS_FUSE_LOGLEVEL=<n> to set the log leve different from LOG_INFO
  //----------------------------------------------------------------------------
  diamond::common::Logging::Init();
  diamond::common::Logging::SetUnit("FUSE/RadosFS");
  diamond::common::Logging::gShortFormat = true;
  std::string fusedebug = getenv("RADOSFS_FUSE_DEBUG")?getenv("RADOSFS_FUSE_DEBUG"):"0";
  
  if ((getenv("RADOSFS_FUSE_DEBUG")) && (fusedebug != "0"))
  {
    diamond::common::Logging::SetLogPriority(LOG_DEBUG);
  }
  else
  { 
    if ((getenv("RADOSFS_FUSE_LOGLEVEL"))) 
    {
      diamond::common::Logging::SetLogPriority(atoi(getenv("RADOSFS_FUSE_LOGLEVEL")));
    } 
    else 
    {
      diamond::common::Logging::SetLogPriority(LOG_INFO);
    }
  }

  //----------------------------------------------------------------------------
  // start the FUSE daemon
  //----------------------------------------------------------------------------
  return fs.daemonize(argc, argv, &fs, NULL);
}
