set(TARGET_NAME Akva)

include_directories(${Boost_INCLUDE_DIRS})

# collect sources
set(TARGET_SOURCES ../akvarium.cpp
					  declaration.hpp
					  manager.cpp
					  manager.hpp
					  action.cpp
					  action.hpp
					  driver.cpp
					  driver.hpp
					  akva.cpp
					  akva.hpp)


file(GLOB SRC_ENGINE ${AKVA_KERNEL_DIR}/spin_lock.[hc]pp
					 ${AKVA_KERNEL_DIR}/detail/sleeper.[hc]pp
					 ${AKVA_KERNEL_DIR}/mail.[hc]pp
					 ${AKVA_KERNEL_DIR}/message.[hc]pp
					 ${AKVA_KERNEL_DIR}/block.[hc]pp
					 ${AKVA_KERNEL_DIR}/block_environment.[hc]pp
					 ${AKVA_KERNEL_DIR}/regular_registr.[hc]pp
					 ${AKVA_KERNEL_DIR}/block_interface.hpp
					 ${AKVA_KERNEL_DIR}/protocol.[hc]pp
					 ${AKVA_KERNEL_DIR}/buffer.[hc]pp
					 ${AKVA_KERNEL_DIR}/regedit.[hc]pp)

file(GLOB SRC_SUPPORT ${AKVA_SUPPORT_DIR}/logger.[hc]pp
					  ${AKVA_SUPPORT_DIR}/notify_backend.[hc]pp
					  ${AKVA_SUPPORT_DIR}/notify_to_console.[hc]pp
					  ${AKVA_SUPPORT_DIR}/simple_block.[hc]pp
					  ${AKVA_SUPPORT_DIR}/notification_interface.hpp)

source_group("src" FILES ${TARGET_SOURCES})
source_group("src\\support" FILES ${SRC_SUPPORT})
source_group("src\\engine" FILES ${SRC_ENGINE})

add_executable(${TARGET_NAME} ${TARGET_SOURCES} ${SRC_ENGINE} ${SRC_SUPPORT}) 
target_link_libraries(${TARGET_NAME} ${Boost_LIBRARIES})

