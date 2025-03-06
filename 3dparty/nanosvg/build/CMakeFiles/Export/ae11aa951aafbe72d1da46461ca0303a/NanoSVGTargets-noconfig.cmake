#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "NanoSVG::nanosvg" for configuration ""
set_property(TARGET NanoSVG::nanosvg APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(NanoSVG::nanosvg PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libnanosvg.a"
  )

list(APPEND _cmake_import_check_targets NanoSVG::nanosvg )
list(APPEND _cmake_import_check_files_for_NanoSVG::nanosvg "${_IMPORT_PREFIX}/lib/libnanosvg.a" )

# Import target "NanoSVG::nanosvgrast" for configuration ""
set_property(TARGET NanoSVG::nanosvgrast APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(NanoSVG::nanosvgrast PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libnanosvgrast.a"
  )

list(APPEND _cmake_import_check_targets NanoSVG::nanosvgrast )
list(APPEND _cmake_import_check_files_for_NanoSVG::nanosvgrast "${_IMPORT_PREFIX}/lib/libnanosvgrast.a" )

# Import target "NanoSVG::NanoSVGApp" for configuration ""
set_property(TARGET NanoSVG::NanoSVGApp APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(NanoSVG::NanoSVGApp PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/bin/NanoSVGApp"
  )

list(APPEND _cmake_import_check_targets NanoSVG::NanoSVGApp )
list(APPEND _cmake_import_check_files_for_NanoSVG::NanoSVGApp "${_IMPORT_PREFIX}/bin/NanoSVGApp" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
