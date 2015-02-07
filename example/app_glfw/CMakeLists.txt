cmake_minimum_required (VERSION 2.8.12)

set (PROJECT appGlfwExample)
project (${PROJECT})

set (SOURCES
    main.cpp)

include_directories(../../core_source/include)

find_package(OpenGL REQUIRED)

if (MSVC)
    add_executable(${PROJECT} ${SOURCES})
    set_target_properties(${PROJECT} PROPERTIES
                        LINK_FLAGS "/ENTRY:mainCRTStartup")
else()
    add_executable(${PROJECT} ${SOURCES})
endif()

target_link_libraries(${PROJECT} glfw ${GLFW_LIBRARIES} gl_core ${OPENGL_LIBRARIES} mlogger)