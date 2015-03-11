include_directories( ../ )


link_directories( ${CMAKE_INSTALL_FULL_LIBDIR}
		)

add_library( diamond_common SHARED
  Logging.cc
  RWMutex.cc
  hash/map128.cc
  hash/spooky.cc
)

target_link_libraries( diamond_common pthread
)


if (Linux)
  set_target_properties (diamond_common PROPERTIES
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    SOVERSION ${VERSION_MAJOR}
    CLEAN_DIRECT_OUTPUT 1
  )
endif(Linux)

install ( TARGETS diamond_common
          LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
          RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)

