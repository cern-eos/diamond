include_directories( ../ )


link_directories( ${CMAKE_INSTALL_FULL_LIBDIR}
		)

add_library( diamond_rio SHARED
  diamondCache.cc
  diamondFile.cc
  diamondDir.cc
  diamondMeta.cc
  diamondBackend/PosixBackend.cc
  diamondBackend/diamondBackend.cc
)

target_link_libraries( diamond_rio pthread rt
)


if (Linux)
  set_target_properties (diamond_rio PROPERTIES
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    SOVERSION ${VERSION_MAJOR}
    CLEAN_DIRECT_OUTPUT 1
  )
endif(Linux)

install ( TARGETS diamond_rio
          LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
          RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)

