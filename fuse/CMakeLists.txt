find_package (FUSE REQUIRED)
include_directories (${FUSE_INCLUDE_DIRS} "../fuse" "../" "/usr/local/include/osxfuse/fuse")

add_executable (diamondfs
                diamondfs.cc)

link_directories(${LIBS_DIR} ${FUSE_LINK_DIR} "/usr/local/lib")

target_link_libraries (diamondfs
    ${FUSE_LDFLAGS}
#    -losxfuse  # FIXME!
     diamond_common diamond_rio
)

install (TARGETS diamondfs DESTINATION bin)
