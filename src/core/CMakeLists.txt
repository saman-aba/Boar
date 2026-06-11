add_library(libboar STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/appconfig.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/appconfig.h
)
target_include_directories(libboar PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(libboar PUBLIC
	Qt6::Core
)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/diameter.c AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/diameter.h AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/avp.h AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../common.h AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../val-str.h)
	target_sources(libboar PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/diameter.c
		${CMAKE_CURRENT_SOURCE_DIR}/diameter.h
	)
	target_include_directories(libboar PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/..
	)
endif()
