find_package (FUSE REQUIRED)
include_directories (${FUSE_INCLUDE_DIRS} "../fuse" "../" "/usr/local/include/osxfuse/fuse")

add_executable (radosfs
                radosfs.cc)

link_directories(${LIBS_DIR} ${FUSE_LINK_DIR} "/usr/local/lib")

target_link_libraries (radosfs
    ${FUSE_LDFLAGS}
#    -losxfuse  # FIXME!
     DiamondCommon
)

install (TARGETS radosfs DESTINATION bin)
