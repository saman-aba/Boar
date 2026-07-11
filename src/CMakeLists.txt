

add_subdirectory(core)
add_subdirectory(ui)

add_library(core OBJECT
	./main.cpp
	./launcher.cpp
	./sandbox.cpp
	./platform/linux/launcher_linux.cpp
)

target_include_directories(core PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
)
#target_link_libraries(qtui PUBLIC )
target_link_libraries(core PUBLIC
	Qt6::Widgets)
