cmake_minimum_required (VERSION 2.8.12)

set (PROJECT CG3dot11)
project (${PROJECT})

set(CMAKE_AUTOMOC ON)
set(AUTOMOC_MOC_OPTIONS PROPERTIES FOLDER CMakeAutomocTargets)
set_property(GLOBAL PROPERTY AUTOMOC_FOLDER CMakeAutomocTargets)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Core    REQUIRED)
find_package(Qt5Gui     REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(OpenGL REQUIRED)

if (NOT Qt5Core_FOUND)
	message("Example ${target} skipped: Qt5 not found")
	return()
endif()

file(GLOB HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.h")
file(GLOB SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.cpp")
file(GLOB QML_LIST RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.qml")
file(GLOB QRC_LIST RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.qrc")

include_directories(../../core_source/include)

qt5_add_resources(QRC_LIST ${QRC_LIST})
add_executable(${PROJECT} WIN32 ${HEADERS} ${SOURCES} ${QML_LIST} ${QRC_LIST})
target_link_libraries(${PROJECT} gl_core ${OPENGL_LIBRARIES} mlogger)

qt5_use_modules(${PROJECT} Qml Quick Widgets)
