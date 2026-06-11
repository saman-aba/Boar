add_library(core INTERFACE)
target_include_directories(core INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}
)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/diameter.c AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/diameter.h AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/avp.h AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../common.h AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../val-str.h)
	add_library(core_diameter OBJECT
		${CMAKE_CURRENT_SOURCE_DIR}/diameter.c
		${CMAKE_CURRENT_SOURCE_DIR}/diameter.h
	)
	target_include_directories(core_diameter PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/..
	)
	target_link_libraries(core INTERFACE core_diameter)
endif()
