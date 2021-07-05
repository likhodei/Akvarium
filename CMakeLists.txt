# -*- mode: cmake; coding: utf-8 -*-
cmake_minimum_required(VERSION 3.16)
set(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE TRUE)

set(AKVA_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

project(Akvarium)

SET(CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	${PROJECT_SOURCE_DIR}
	${PROJECT_SOURCE_DIR}/cmake
)

set(VERSION_SERIES 1)
set(VERSION_MAJOR 2)
set(VERSION_MINOR 3)

set(AKVA_SOVERSION 123)

option(USE_TEST "Use TEST" ON)

# Various things that must be configured by the user or packager ...

if(NOT AKVA_BINARY_DIR)
	set(AKVA_BINARY_DIR ${PROJECT_SOURCE_DIR}/bin)
	message(STATUS "BIN-DIR [DEFAULT]: " ${AKVA_BINARY_DIR})
else()
	message(STATUS "BIN-DIR [BY USER]: " ${AKVA_BINARY_DIR})
endif()

set("${PROJECT}_BINARY_DIR" ${AKVA_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${${PROJECT}_BINARY_DIR})
set(EXECUTABLE_OUTPUT_PATH ${${PROJECT}_BINARY_DIR})

link_directories(${AKVA_BINARY_DIR})

set(DEBUG_LIB_POSTFIX "d" CACHE STRING "String appended to debug libraries")
set(LIB_INSTALL_DIR "lib" CACHE STRING "Name for library directory within ${CMAKE_INSTALL_PREFIX}")

# C++11 vs C++98
if(WIN32)
	if(MSVC)
		if(MSVC_VERSION GREATER 1600)
			set(HAS_CXX11 ON)
		endif(MSVC_VERSION GREATER 1600)

		set(BUILD_PARALLEL "/MP" CACHE STRING "MSVC option for parallel builds (/MP or /MPx)")
	endif(MSVC)

	set(CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS} /O2 /MD /Zi")
#	set(CMAKE_LINK_FLAGS_RELEASE  "${CMAKE_LINK_FLAGS} /DEBUG /OPT:REF /MAP /MAPINFO:EXPORTS")

	set(CMAKE_EXE_LINKER_FLAGS_RELEASE  "${CMAKE_LINK_FLAGS} /INCREMENTAL:NO /DEBUG /OPT:REF")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /INCREMENTAL:NO /DEBUG /OPT:REF")

	message(STATUS "FLAGS: ${CMAKE_LINK_FLAGS_RELEASE}")

else(WIN32)
	message(FATAL_ERROR "ToDo: ... linux, other, ... ")
endif(WIN32)

# option(DEBUG "Support for debugging" OFF)

# +++ setup boost libraries +++

find_package(Boost REQUIRED)
find_package(RapidJson)
find_package(RapidXML)

if(Boost_FOUND)
	message(STATUS "Boost version: " ${Boost_VERSION} " Location: " ${BOOST_ROOT})
	link_directories(${BOOST_LIBRARYDIR})
else()
	message(FATAL_ERROR "!!!! NO BOOST !!!!")
endif()

# +++ setup other libs +++

add_subdirectory(src)

if(USE_TEST)
# add_subdirectory(tests)
endif()

