qt6_wrap_ui(Boar_WIDGET_UI_HEADERS
#	${CMAKE_CURRENT_SOURCE_DIR}/DiameterWidget.ui
#	${CMAKE_CURRENT_SOURCE_DIR}/TcapWidget.ui
#	${CMAKE_CURRENT_SOURCE_DIR}/GtpWidget.ui
	${CMAKE_CURRENT_SOURCE_DIR}/PacketEditorModuleWidget.ui
	${CMAKE_CURRENT_SOURCE_DIR}/DiameterEditorWidget.ui
	${CMAKE_CURRENT_SOURCE_DIR}/TcapEditorWidget.ui
	${CMAKE_CURRENT_SOURCE_DIR}/DashboardModuleWidget.ui
	${CMAKE_CURRENT_SOURCE_DIR}/NodeEmulatorModuleWidget.ui
	${CMAKE_CURRENT_SOURCE_DIR}/DatabaseModuleWidget.ui
	${CMAKE_CURRENT_SOURCE_DIR}/MessagingModuleWidget.ui
	${CMAKE_CURRENT_SOURCE_DIR}/AnalyzerModuleWidget.ui
	${CMAKE_CURRENT_SOURCE_DIR}/SettingsModuleWidget.ui
)

add_library(qtui_widgets OBJECT
#	${CMAKE_CURRENT_SOURCE_DIR}/diameterwidget.cpp
#	${CMAKE_CURRENT_SOURCE_DIR}/tcapwidget.cpp
#	${CMAKE_CURRENT_SOURCE_DIR}/gtpwidget.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/packeteditormodulewidget.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/diametereditorwidget.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/tcapeditorwidget.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/dashboardmodulewidget.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/nodeemulatormodulewidget.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/databasemodulewidget.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/messagingmodulewidget.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/analyzermodulewidget.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/settingsmodulewidget.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/diameterwidget.h
	${CMAKE_CURRENT_SOURCE_DIR}/tcapwidget.h
	${CMAKE_CURRENT_SOURCE_DIR}/gtpwidget.h
	${CMAKE_CURRENT_SOURCE_DIR}/packeteditormodulewidget.h
	${CMAKE_CURRENT_SOURCE_DIR}/diametereditorwidget.h
	${CMAKE_CURRENT_SOURCE_DIR}/tcapeditorwidget.h
	${CMAKE_CURRENT_SOURCE_DIR}/dashboardmodulewidget.h
	${CMAKE_CURRENT_SOURCE_DIR}/nodeemulatormodulewidget.h
	${CMAKE_CURRENT_SOURCE_DIR}/databasemodulewidget.h
	${CMAKE_CURRENT_SOURCE_DIR}/messagingmodulewidget.h
	${CMAKE_CURRENT_SOURCE_DIR}/analyzermodulewidget.h
	${CMAKE_CURRENT_SOURCE_DIR}/settingsmodulewidget.h
	${Boar_WIDGET_UI_HEADERS}
)

target_include_directories(qtui_widgets PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_SOURCE_DIR}/../..
	${CMAKE_CURRENT_SOURCE_DIR}/../../core
	${CMAKE_CURRENT_SOURCE_DIR}/../dialogs
	${CMAKE_CURRENT_BINARY_DIR}/../dialogs
)
target_link_libraries(qtui_widgets PUBLIC
	Qt6::Widgets
)
target_compile_definitions(qtui_widgets PUBLIC
	BOAR_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)
