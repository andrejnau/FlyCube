cmake_minimum_required (VERSION 2.8.12)

set (PROJECT appQtExample)
project (${PROJECT})

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Core    REQUIRED)
find_package(Qt5Gui     REQUIRED)
find_package(Qt5Widgets REQUIRED)

if (NOT Qt5Core_FOUND)
    message("Example ${target} skipped: Qt5 not found")
    return()
endif()

file(GLOB HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.h")
file(GLOB SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.cpp")
file(GLOB UI_LIST RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.ui")

include_directories(../../core_source/include)

qt5_wrap_ui(${PROJECT} ${UI_LIST})
add_executable(${PROJECT} WIN32 ${HEADERS} ${SOURCES} ${UI_LIST})

target_link_libraries(${PROJECT} gl_core mlogger)
qt5_use_modules(${PROJECT} Core Gui Widgets)