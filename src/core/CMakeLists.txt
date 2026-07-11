
find_package(PkgConfig REQUIRED)
pkg_check_modules(JANSSON REQUIRED jansson)

add_library(libboar STATIC
	./appconfig.cpp
	./appconfig.h
	./db.c
	./db.h
	./sqlite3.c
	./sqlite3.h
	./packet-builder.c
	./packet-builder.h
	./diameter.c
	./diameter.h
	./asn1-primitives.c
	./asn1.h
	./generated/tcap.c
	./generated/tcap.h
)
target_include_directories(libboar PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${JANSSON_INCLUDE_DIRS}
)
target_link_libraries(libboar PUBLIC
	Qt6::Core
	${JANSSON_LIBRARIES}
)

#add_subdirectory(map)
#add_subdirectory(tcap)
#add_subdirectory(camel)

target_sources(libboar PRIVATE
)
target_include_directories(libboar PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_SOURCE_DIR}/generated
)
