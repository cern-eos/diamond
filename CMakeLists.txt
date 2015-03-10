cmake_minimum_required(VERSION 2.8)
project(Diamond)

set (VERSION_MAJOR 1)
set (VERSION_MINOR 0)
set (VERSION_PATCH 0)
set (RELEASE 0)
set (VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

# force /usr/ installation prefix
set(CMAKE_INSTALL_PREFIX /usr)
set(CMAKE_INSTALL_SYSCONFDIR /etc)
set(CMAKE_INSTALL_SBINDIR /sbin)

# include GNU path settings
include (GNUInstallDirs)

option (BUILD_SHARED_LIBS "Build dynamic link libraries" OFF)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    "${CMAKE_SOURCE_DIR}/CMakeModules/")

set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")

set (CPACK_PACKAGE_NAME "Diamond")
set (CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set (CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
set (CPACK_SOURCE_GENERATOR TGZ)
set (CPACK_SOURCE_IGNORE_FILES
    "/_darcs/"
    "/build/")
include (CPack)

find_package (OpenSSL REQUIRED)

add_definitions (-D_FILE_OFFSET_BITS=64 -DLinux )
if (APPLE)
    add_definitions (-D__FreeBSD__=10 -std=gnu++0x)
endif (APPLE)

set(CMAKE_CXX_FLAGS "-Wall -Wno-deprecated-declarations -mcx16 -std=gnu++0x -DVERSION=\\\"${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}\\\" -DRELEASE=\\\"${RELEASE}\\\"" )

include_directories (${DIAMOND_BINARY_DIR})
include_directories (${DIAMOND_SOURCE_DIR})

add_subdirectory(common)
add_subdirectory(rio)
add_subdirectory(fuse)
add_subdirectory(test)

######################################################################################################################
# Print Configuration
######################################################################################################################

message( STATUS "_________________________________________________" )
message( STATUS "Version           : diamond-" ${VERSION_MAJOR} "." ${VERSION_MINOR} "." ${VERSION_PATCH} "-" ${RELEASE} )
message( STATUS "................................................." )
message( STATUS "prefix            : " ${CMAKE_INSTALL_PREFIX} )
message( STATUS "bin dir           : " ${CMAKE_INSTALL_FULL_BINDIR} )
message( STATUS "sbin dir          : " ${CMAKE_INSTALL_SBINDIR} )
message( STATUS "lib dir           : " ${CMAKE_INSTALL_FULL_LIBDIR} )
message( STATUS "sysconfig dir     : " ${CMAKE_INSTALL_SYSCONFDIR} )
message( STATUS "................................................." )
message( STATUS "C Compiler        : " ${CMAKE_C_COMPILER} )
message( STATUS "C++ Compiler      : " ${CMAKE_CXX_COMPILER} )
message( STATUS "Build type        : " ${CMAKE_BUILD_TYPE} )
message( STATUS "_________________________________________________" )
