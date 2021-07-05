set(AKVA_SUPPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/support)
set(AKVA_KERNEL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/engine)
set(AKVA_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/akva)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if(Boost_FOUND)
	add_definitions(-DBOOST_CHRONO_HEADER_ONLY)
endif()

add_definitions(-DWIN32_LEAN_AND_MEAN)

add_subdirectory(akva)
