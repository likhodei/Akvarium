set(TARGET_NAME Akva)

include_directories(${Boost_INCLUDE_DIRS})

# collect sources
set(TARGET_SRC ../akvarium.cpp
				  declaration.hpp
				  manager.cpp
				  manager.hpp
				  action.cpp
				  action.hpp
				  driver.cpp
				  driver.hpp
				  akva.cpp
				  akva.hpp)

file(GLOB SRC_ENGINE ${AKVA_ENGINE}/spin_lock.[hc]pp
					 ${AKVA_ENGINE}/detail/sleeper.[hc]pp
					 ${AKVA_ENGINE}/mail.[hc]pp
					 ${AKVA_ENGINE}/message.[hc]pp
					 ${AKVA_ENGINE}/frame.[hc]pp
					 ${AKVA_ENGINE}/memory.[hc]pp
					 ${AKVA_ENGINE}/regular_registr.[hc]pp
					 ${AKVA_ENGINE}/allocator.hpp
					 ${AKVA_ENGINE}/protocol.[hc]pp
					 ${AKVA_ENGINE}/buffer.[hc]pp
					 ${AKVA_ENGINE}/regedit.[hc]pp
					 ${AKVA_ENGINE}/logger.[hc]pp
					 ${AKVA_ENGINE}/notification.[hc]pp
					 ${AKVA_ENGINE}/terminal.[hc]pp
					 ${AKVA_ENGINE}/block.[hc]pp
					 ${AKVA_ENGINE}/bits.hpp
					 )

source_group("src" FILES ${TARGET_SRC})
source_group("src\\engine" FILES ${SRC_ENGINE})

add_executable(${TARGET_NAME} ${TARGET_SRC} ${SRC_ENGINE}) 
target_link_libraries(${TARGET_NAME} ${Boost_LIBRARIES})

