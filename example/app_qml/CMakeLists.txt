set (PROJECT appQmlExample)
project (${PROJECT})

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
target_link_libraries(${PROJECT} gl_core mlogger)
qt5_use_modules(${PROJECT} Qml Quick Widgets)