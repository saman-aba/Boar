cmake_minimum_required( VERSION 3.20)

set( _project_name Boar)

project( ${_project_name} C CXX)

set( PROJECT_MAJOR_VERSION 0)
set( PROJECT_MINOR_VERSION 1)

set( PROJECT_VERSION "${PROJECT_MAJOR_VERSION}.${PROJECT_MINOR_VERSION}")

set( CMAKE_C_STANDARD 11)
set( CMAKE_C_STANDARD_REQUIRED ON)

set( CMAKE_CXX_STANDARD 20)
set( CMAKE_CXX_STANDARD_REQUIRED ON)
set( CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC ON)

option(USE_qt6 "qt based gui" ON)

set(CMAKE_BUILD_TYPE "Debug")

if(USE_qt6)
	list( APPEND CMAKE_PREFIX_PATH ${QT6_PREFIX_PATH})
	find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
	add_subdirectory(src)
	add_executable(Boar
		$<TARGET_OBJECTS:core>
		$<TARGET_OBJECTS:qtui>
		$<TARGET_OBJECTS:qtui_dialogs>
		$<TARGET_OBJECTS:qtui_widgets>
	)
	target_link_libraries(Boar PRIVATE libboar qtui qtui_dialogs qtui_widgets)

	install(TARGETS Boar
		RUNTIME DESTINATION bin
	)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/app_icon.png
		DESTINATION share/pixmaps
		RENAME boar.png
	)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/packaging/boar.desktop
		DESTINATION share/applications
	)
endif()

set(CPACK_PACKAGE_NAME "boar")
set(CPACK_PACKAGE_VENDOR "Boar")
set(CPACK_PACKAGE_CONTACT "Boar Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Qt6 telecom packet generation and analysis application")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SECTION "net")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
	"${CMAKE_CURRENT_SOURCE_DIR}/packaging/postinst"
	"${CMAKE_CURRENT_SOURCE_DIR}/packaging/postrm"
)

include(CPack)
