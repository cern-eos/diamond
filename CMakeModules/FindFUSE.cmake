# Configure FUSE using pkgconfig

FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(FUSE REQUIRED fuse)

