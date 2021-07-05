# This script locates the RapidJson library
# ------------------------------------
#
# usage:
# find_package(RapidJson ...)
#
# searches in RAPIDJSON_ROOT and usual locations
#
# Sets RAPIDJSON_DIR

# find the tinyxml directory find the SFML include directory

if(NOT RAPIDJSON_ROOT)
    if(NOT $ENV{RAPIDJSON_ROOT} STREQUAL "")
        set(RAPIDJSON_ROOT "$ENV{RAPIDJSON_ROOT}")
    else()
        if(WIN32)
            message("Please set RAPIDJSON_ROOT to RapidJson directory")
        endif()
    endif()
endif()

# Look for the header file.
find_path(RapidJson_INCLUDE_DIR NAMES rapidjson.h HINTS ${RAPIDJSON_ROOT} PATH_SUFFIXES rapidjson)

if(NOT ${RapidJson_INCLUDE_DIR} STREQUAL "")
	set(RapidJson_INCLUDE_DIR "${RAPIDJSON_ROOT}")
endif()

# handle the QUIETLY and REQUIRED arguments and set RapidXML_FOUND to TRUE if 
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RapidJson FOUND_VAR RapidJson_FOUND REQUIRED_VARS RapidJson_INCLUDE_DIR)

# Copy the results to the output variables.
if(RapidJson_FOUND)
  message("RapidJson found")
  set(RapidJson_INCLUDE_DIRS ${RapidJson_INCLUDE_DIR})
else()
  set(RapidJson_INCLUDE_DIRS)
endif()

mark_as_advanced(RapidJson_INCLUDE_DIR)
