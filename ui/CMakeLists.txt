

qt6_wrap_ui(Boar_UI_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/MainWindow.ui
)

add_library(qtui OBJECT
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.h
	${CMAKE_CURRENT_SOURCE_DIR}/boar_mainwindow.h
	${Boar_UI_HEADERS}
)

target_include_directories(qtui PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(qtui PUBLIC
	Qt6::Widgets
)
