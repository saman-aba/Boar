

add_subdirectory(core)
add_subdirectory(ui)

add_library(app OBJECT
	${CMAKE_CURRENT_SOURCE_DIR}/launcher.cpp
)

target_include_directories(app PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
)
#target_link_libraries(qtui PUBLIC )
target_link_libraries(app PUBLIC
	Qt6::Widgets)
