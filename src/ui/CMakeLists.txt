add_subdirectory(dialogs)
add_subdirectory(widgets)

qt6_wrap_ui(Boar_UI_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/MainWindow.ui
	${CMAKE_CURRENT_SOURCE_DIR}/PacketGeneratorWindow.ui
	${CMAKE_CURRENT_SOURCE_DIR}/DiameterEditorWidget.ui
	${CMAKE_CURRENT_SOURCE_DIR}/TcapEditorWidget.ui
)

add_library(qtui OBJECT
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.h
	${CMAKE_CURRENT_SOURCE_DIR}/boar_mainwindow.h
	${CMAKE_CURRENT_SOURCE_DIR}/packetgeneratorwindow.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/packetgeneratorwindow.h
	${CMAKE_CURRENT_SOURCE_DIR}/diametereditorwidget.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/diametereditorwidget.h
	${CMAKE_CURRENT_SOURCE_DIR}/tcapeditorwidget.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/tcapeditorwidget.h
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
