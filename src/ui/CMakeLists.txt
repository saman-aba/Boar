add_subdirectory(dialogs)
add_subdirectory(widgets)

qt6_wrap_ui(Boar_UI_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/MainWindow.ui
)

add_library(qtui OBJECT
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/apptheme.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/apptheme.h
	${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc
	${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.h
	${CMAKE_CURRENT_SOURCE_DIR}/forgedpacketrecord.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/forgedpacketrecord.h
	${CMAKE_CURRENT_SOURCE_DIR}/forgedpacketrepository.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/forgedpacketrepository.h
	${Boar_UI_HEADERS}
	$<TARGET_OBJECTS:qtui_dialogs>
	$<TARGET_OBJECTS:qtui_widgets>
)

target_include_directories(qtui PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/dialogs
	${CMAKE_CURRENT_SOURCE_DIR}/widgets
	${CMAKE_CURRENT_BINARY_DIR}/dialogs
	${CMAKE_CURRENT_BINARY_DIR}/widgets
)
target_link_libraries(qtui PUBLIC
	Qt6::Widgets
	libboar
	qtui_dialogs
	qtui_widgets
)
