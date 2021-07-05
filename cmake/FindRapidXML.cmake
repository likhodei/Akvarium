# This script locates the RapidXML library
# ------------------------------------
#
# usage:
# find_package(RAPIDXML ...)
#
# searches in RAPIDXML_ROOT and usual locations
#
# Sets RAPIDXML_DIR

# find the tinyxml directory find the SFML include directory

if(NOT RAPIDXML_ROOT)
    if(NOT $ENV{RAPIDXML_ROOT} STREQUAL "")
        set(RAPIDXML_ROOT "$ENV{RAPIDXML_ROOT}")
    else()
        if(WIN32)
            message("Please set RAPIDXML_ROOT to MessagePack directory")
        endif()
    endif()
endif()

# Look for the header file.
find_path(RapidXML_INCLUDE_DIR NAMES rapidxml.hpp HINTS ${RAPIDXML_ROOT})

# handle the QUIETLY and REQUIRED arguments and set RapidXML_FOUND to TRUE if 
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RapidXML FOUND_VAR RapidXML_FOUND REQUIRED_VARS RapidXML_INCLUDE_DIR)

# Copy the results to the output variables.
if(RapidXML_FOUND)
  message("RapidXML found")
  set(RapidXML_INCLUDE_DIRS ${RapidXML_INCLUDE_DIR})
else()
  set(RapidXML_INCLUDE_DIRS)
endif()

mark_as_advanced(RapidXML_INCLUDE_DIR)
