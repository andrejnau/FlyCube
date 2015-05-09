set (PROJECT appQtExample)
project (${PROJECT})

if (NOT Qt5Core_FOUND)
	message("${PROJECT} skipped: Qt5 not found")
	return()
endif()

file(GLOB HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.h")
file(GLOB SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.cpp")
file(GLOB UI_LIST RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.ui")

qt5_wrap_ui(${PROJECT} ${UI_LIST})
add_executable(${PROJECT} WIN32 ${HEADERS} ${SOURCES} ${UI_LIST})

target_link_libraries(${PROJECT} gl_core mlogger)
qt5_use_modules(${PROJECT} Core Gui Widgets)