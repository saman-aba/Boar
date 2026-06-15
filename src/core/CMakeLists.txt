add_library(libboar STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/appconfig.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/appconfig.h
)
target_include_directories(libboar PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${JANSSON_INCLUDE_DIRS}
)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JANSSON REQUIRED jansson)

target_link_libraries(libboar PUBLIC
	Qt6::Core
	${JANSSON_LIBRARIES}
)

target_sources(libboar PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/diameter.c
	${CMAKE_CURRENT_SOURCE_DIR}/diameter.h
)
target_include_directories(libboar PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/..
)
