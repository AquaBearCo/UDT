#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "udt::udt" for configuration "Release"
set_property(TARGET udt::udt APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(udt::udt PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libudt.so"
  IMPORTED_SONAME_RELEASE "libudt.so"
  )

list(APPEND _cmake_import_check_targets udt::udt )
list(APPEND _cmake_import_check_files_for_udt::udt "${_IMPORT_PREFIX}/lib/libudt.so" )

# Import target "udt::udt_static" for configuration "Release"
set_property(TARGET udt::udt_static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(udt::udt_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libudt.a"
  )

list(APPEND _cmake_import_check_targets udt::udt_static )
list(APPEND _cmake_import_check_files_for_udt::udt_static "${_IMPORT_PREFIX}/lib/libudt.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
