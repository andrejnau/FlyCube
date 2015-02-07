cmake_minimum_required (VERSION 2.8.12)

set (PROJECT FlyCube)
project (${PROJECT})

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

if (NOT MSVC)
    set (CMAKE_CXX_FLAGS "-std=c++11")
else()
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

#glfw
add_subdirectory(3rdparty/glfw-3.1)
include_directories(3rdparty/glfw-3.1/include)
add_definitions("-DGLFW_INCLUDE_NONE")

#glm
include_directories(3rdparty/glm)

#glbinding
#add_subdirectory(3rdparty/glbinding-master)
#include_directories(3rdparty/glbinding-master/source/glbinding/include)
#add_definitions(-DGLBINDING_STATIC)
#add_definitions(-DGLBINDING_EXPORTS)

#gl_core
set (HEADERS_gl_core
    3rdparty/gl_core/gl_core.h
    3rdparty/gl_core/gl_core_4_3.h)

set (SOURCES_gl_core
    3rdparty/gl_core/gl_core_4_3.c)

include_directories(3rdparty/gl_core)
add_library(gl_core ${HEADERS_gl_core} ${SOURCES_gl_core})

#project
add_definitions(-DPROJECT_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/core_source/res/")
add_subdirectory(core_source)
add_subdirectory(example)
add_subdirectory(learning_tasks)