set(AKVA_CORE ${CMAKE_CURRENT_SOURCE_DIR}/akva)
set(AKVA_ENGINE ${CMAKE_CURRENT_SOURCE_DIR}/engine)

#set(AKVA_PGRL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/pgrl)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if(Boost_FOUND)
	add_definitions(-DBOOST_CHRONO_HEADER_ONLY)
endif()

add_definitions(-DWIN32_LEAN_AND_MEAN)

add_subdirectory(akva)
#add_subdirectory(fish)

