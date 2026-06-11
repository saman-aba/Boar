qt6_wrap_ui(Boar_DIALOG_UI_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/ConfigDialog.ui
	${CMAKE_CURRENT_SOURCE_DIR}/NewPacketDialog.ui
	${CMAKE_CURRENT_SOURCE_DIR}/AddAvpDialog.ui
)

add_library(qtui_dialogs OBJECT
	${CMAKE_CURRENT_SOURCE_DIR}/configdialog.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/newpacketdialog.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/addavpdialog.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/configdialog.h
	${CMAKE_CURRENT_SOURCE_DIR}/newpacketdialog.h
	${CMAKE_CURRENT_SOURCE_DIR}/addavpdialog.h
	${Boar_DIALOG_UI_HEADERS}
)

target_include_directories(qtui_dialogs PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_SOURCE_DIR}/../widgets
	${CMAKE_CURRENT_BINARY_DIR}/../widgets
)
target_link_libraries(qtui_dialogs PUBLIC
	Qt6::Widgets
)
