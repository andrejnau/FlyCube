set (PROJECT CG1dot13)
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
file(GLOB QML_LIST RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.qml")
file(GLOB QRC_LIST RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.qrc")

qt5_add_resources(QRC_LIST ${QRC_LIST})
add_executable(${PROJECT} WIN32 ${HEADERS} ${SOURCES} ${QML_LIST} ${QRC_LIST})
target_link_libraries(${PROJECT} gl_core mlogger)

qt5_use_modules(${PROJECT} Qml Quick Widgets)
set_target_properties(${PROJECT} PROPERTIES FOLDER "learning")