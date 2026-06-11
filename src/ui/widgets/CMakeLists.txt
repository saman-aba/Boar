qt6_wrap_ui(Boar_WIDGET_UI_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/DiameterWidget.ui
	${CMAKE_CURRENT_SOURCE_DIR}/TcapWidget.ui
	${CMAKE_CURRENT_SOURCE_DIR}/GtpWidget.ui
)

add_library(qtui_widgets OBJECT
	${CMAKE_CURRENT_SOURCE_DIR}/diameterwidget.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/tcapwidget.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/gtpwidget.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/diameterwidget.h
	${CMAKE_CURRENT_SOURCE_DIR}/tcapwidget.h
	${CMAKE_CURRENT_SOURCE_DIR}/gtpwidget.h
	${Boar_WIDGET_UI_HEADERS}
)

target_include_directories(qtui_widgets PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_SOURCE_DIR}/../dialogs
	${CMAKE_CURRENT_BINARY_DIR}/../dialogs
)
target_link_libraries(qtui_widgets PUBLIC
	Qt6::Widgets
)
