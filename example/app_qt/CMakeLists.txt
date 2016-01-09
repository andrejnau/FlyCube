set (PROJECT appQtExample)
project (${PROJECT})

if (Qt5Core_FOUND)
    set(CMAKE_AUTOMOC ON)
    set(AUTOMOC_MOC_OPTIONS PROPERTIES FOLDER CMakeAutomocTargets)
    set_property(GLOBAL PROPERTY AUTOMOC_FOLDER CMakeAutomocTargets)
else()
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

target_link_libraries(${PROJECT} ${GLAD_LIBRARIES} mlogger)
qt5_use_modules(${PROJECT} Core Gui Widgets)

set_target_properties(${PROJECT} PROPERTIES FOLDER "example")