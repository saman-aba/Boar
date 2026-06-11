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

if(USE_qt6)
	list( APPEND CMAKE_PREFIX_PATH ${QT6_PREFIX_PATH})
	find_package(Qt6 REQUIRED COMPONENTS Widgets)
	add_subdirectory(src)
	add_executable(Boar
		$<TARGET_OBJECTS:qtui>
		$<TARGET_OBJECTS:qtui_dialogs>
		$<TARGET_OBJECTS:qtui_widgets>
	)
	target_link_libraries(Boar PRIVATE core qtui qtui_dialogs qtui_widgets)
endif()
