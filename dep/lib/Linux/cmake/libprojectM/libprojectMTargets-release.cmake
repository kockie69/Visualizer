#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libprojectM::static" for configuration "Release"
set_property(TARGET libprojectM::static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(libprojectM::static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C;CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libprojectM.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS libprojectM::static )
list(APPEND _IMPORT_CHECK_FILES_FOR_libprojectM::static "${_IMPORT_PREFIX}/lib/libprojectM.a" )

# Import target "libprojectM::shared" for configuration "Release"
set_property(TARGET libprojectM::shared APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(libprojectM::shared PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libprojectM.so.4.0.0"
  IMPORTED_SONAME_RELEASE "libprojectM.so.4"
  )

list(APPEND _IMPORT_CHECK_TARGETS libprojectM::shared )
list(APPEND _IMPORT_CHECK_FILES_FOR_libprojectM::shared "${_IMPORT_PREFIX}/lib/libprojectM.so.4.0.0" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
