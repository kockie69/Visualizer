#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libprojectM::static" for configuration ""
set_property(TARGET libprojectM::static APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(libprojectM::static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C;CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/liblibprojectM.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS libprojectM::static )
list(APPEND _IMPORT_CHECK_FILES_FOR_libprojectM::static "${_IMPORT_PREFIX}/lib/liblibprojectM.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
