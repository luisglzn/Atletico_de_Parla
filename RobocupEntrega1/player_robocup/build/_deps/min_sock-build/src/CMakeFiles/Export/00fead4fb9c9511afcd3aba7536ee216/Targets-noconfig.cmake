#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "MinimalSocket::MinimalSocket" for configuration ""
set_property(TARGET MinimalSocket::MinimalSocket APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(MinimalSocket::MinimalSocket PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libMinimalSocket.a"
  )

list(APPEND _cmake_import_check_targets MinimalSocket::MinimalSocket )
list(APPEND _cmake_import_check_files_for_MinimalSocket::MinimalSocket "${_IMPORT_PREFIX}/lib/libMinimalSocket.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
