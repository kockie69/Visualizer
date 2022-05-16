#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libprojectM::static" for configuration "Debug"
set_property(TARGET libprojectM::static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(libprojectM::static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C;CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/liblibprojectMd.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS libprojectM::static )
list(APPEND _IMPORT_CHECK_FILES_FOR_libprojectM::static "${_IMPORT_PREFIX}/lib/liblibprojectMd.a" )

# Import target "libprojectM::shared" for configuration "Debug"
set_property(TARGET libprojectM::shared APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(libprojectM::shared PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/libprojectMd.dll.a"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libprojectMd.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS libprojectM::shared )
list(APPEND _IMPORT_CHECK_FILES_FOR_libprojectM::shared "${_IMPORT_PREFIX}/lib/libprojectMd.dll.a" "${_IMPORT_PREFIX}/lib/libprojectMd.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
