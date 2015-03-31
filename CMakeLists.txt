cmake_minimum_required (VERSION 2.8.12)

set (PROJECT FlyCube)
project (${PROJECT})

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    set (CMAKE_CXX_FLAGS "-std=c++11")
endif()

#glfw
add_subdirectory(3rdparty/glfw-3.1)
include_directories(3rdparty/glfw-3.1/include)
add_definitions("-DGLFW_INCLUDE_NONE")

#Simple-OpenGL-Image-Library
add_subdirectory(3rdparty/Simple-OpenGL-Image-Library-master)
include_directories(3rdparty/Simple-OpenGL-Image-Library-master/src)

#glm
include_directories(3rdparty/glm)

#gl_core
add_subdirectory(3rdparty/gl_core)
include_directories(3rdparty/gl_core)

#project
add_definitions(-DPROJECT_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/core_source/res/")
add_definitions(-DPROJECT_RESOURCE_MODEL_DIR="${CMAKE_CURRENT_SOURCE_DIR}/core_source/res/obj-model/")

include_directories(common/mlogger/include)

add_subdirectory(common)
add_subdirectory(core_source)
add_subdirectory(example)
add_subdirectory(learning_tasks)