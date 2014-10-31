//------------------------------------------------------------------------------
// Copyright (c) 2012-2013 by European Organization for Nuclear Research (CERN)
// Author: Justin Salmon <jsalmon@cern.ch>
//------------------------------------------------------------------------------
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with This program.  If not, see <http://www.gnu.org/licenses/>.
//------------------------------------------------------------------------------

#ifndef __LLFUSEXX_H__
#define __LLFUSEXX_H__

#include <sys/stat.h>
#include <sys/types.h>

#ifndef FUSE_USE_VERSION
#ifdef __APPLE__
#define FUSE_USE_VERSION 27
#else
#define FUSE_USE_VERSION 26
//#pragma message("FUSE VERSION 26")
#endif
#endif

extern "C" {
#include <fuse_lowlevel.h>
}

#include <cstdlib>

namespace llfusexx
{
  //----------------------------------------------------------------------------
  //! Interface to the low-level FUSE API
  //----------------------------------------------------------------------------
  template <typename T>
  class fs
  {
    protected:
      //------------------------------------------------------------------------
      //! Allow static methods to access object methods/ variables using 'self'
      //! instead of 'this'
      //------------------------------------------------------------------------
      static T *self;

      //------------------------------------------------------------------------
      //! Structure holding function pointers to the low-level "operations"
      //! (function implementations)
      //------------------------------------------------------------------------
      static struct fuse_lowlevel_ops operations;

      //------------------------------------------------------------------------
      //! @return const reference to the operations struct
      //------------------------------------------------------------------------
      const fuse_lowlevel_ops& get_operations()
      {
        return operations;
      }

    public:
      //------------------------------------------------------------------------
      //! Constructor
      //!
      //! Install pointers to operation functions as implemented by the user
      //! subclass
      //------------------------------------------------------------------------
      fs()
      {
        operations.init         = &T::init;
        operations.destroy      = &T::destroy;
        operations.getattr      = &T::getattr;
        operations.lookup       = &T::lookup;
	operations.setattr      = &T::setattr;
	//        operations.access       = &T::access;
        operations.readdir      = &T::readdir;
	//        operations.mknod        = &T::mknod;
	operations.mkdir        = &T::mkdir;
	operations.unlink       = &T::unlink;
	operations.rmdir        = &T::rmdir;
	operations.rename       = &T::rename;
	operations.open         = &T::open;
	operations.create       = &T::create;
	operations.opendir      = &T::opendir;
	operations.read         = &T::read;
	operations.write        = &T::write;
	operations.statfs       = &T::statfs;
	operations.release      = &T::release;
	operations.releasedir   = &T::releasedir;
	//        operations.fsync        = &T::fsync;
	operations.forget       = &T::forget;
	//        operations.flush        = &T::flush;
	operations.setxattr     = &T::setxattr;
	operations.getxattr     = &T::getxattr;
	operations.listxattr    = &T::listxattr;
	operations.removexattr  = &T::removexattr;
      }

      //------------------------------------------------------------------------
      //! Mount the filesystem with the supplied arguments and run the daemon
      //!
      //! @param argc     commandline argument count
      //! @param argv     commandline argument vector
      //! @param self     reference to user subclass instance, used for
      //!                 convenience within static functions
      //! @param userdata optional extra user data
      //!
      //! @return 0 on success, -1 on error
      //------------------------------------------------------------------------
      int daemonize( int argc, char* argv[], T *self, void *userdata )
      {
        struct fuse_args  args = FUSE_ARGS_INIT(argc, argv);
        struct fuse_chan *channel;
        char             *mountpoint;
        int               error = -1;

        //----------------------------------------------------------------------
        // Parse the commandline and mount the filesystem
        //----------------------------------------------------------------------
        if( fuse_parse_cmdline( &args, &mountpoint, NULL, NULL ) != -1
            && ( channel = fuse_mount( mountpoint, &args ) ) != NULL )
        {
          //--------------------------------------------------------------------
          // The 'self' variable will be the equivalent of the 'this' pointer
          //--------------------------------------------------------------------
          if (self == NULL) {
              return -1;
          }

          T::self = self;

          //--------------------------------------------------------------------
          // Create a new low-level fuse session
          //--------------------------------------------------------------------
          struct fuse_session *session;
          session = fuse_lowlevel_new( &args,
                                       &(get_operations()),
                                       sizeof(fs::operations),
                                       userdata );
          if( session != NULL )
          {
            //------------------------------------------------------------------
            // Set appropriate signal handlers
            //------------------------------------------------------------------
            if( fuse_set_signal_handlers( session ) != -1 )
            {
              //----------------------------------------------------------------
              // Add the mountpoint to the session
              //----------------------------------------------------------------
              fuse_session_add_chan( session, channel );

              //----------------------------------------------------------------
              // Start the multithreaded session loop
              //----------------------------------------------------------------
              error = fuse_session_loop_mt( session );

              //----------------------------------------------------------------
              // Clean up
              //----------------------------------------------------------------
              fuse_remove_signal_handlers( session );
              fuse_session_remove_chan( channel );
            }

            //------------------------------------------------------------------
            // Destroy the session
            //------------------------------------------------------------------
            fuse_session_destroy( session );
          }

          //--------------------------------------------------------------------
          // Unmount the mountpoint
          //--------------------------------------------------------------------
          fuse_unmount( mountpoint, channel );
        }

        //----------------------------------------------------------------------
        // Clean up the fuse arguments
        //----------------------------------------------------------------------
        fuse_opt_free_args( &args );

        return error ? 1 : 0;
      }
  };

  //----------------------------------------------------------------------------
  //! Static declarations
  //----------------------------------------------------------------------------
  template<class T> fuse_lowlevel_ops fs<T>::operations;
  template<class T> T *fs<T>::self;
}

#endif /* __LLFUSEXX_H__ */
