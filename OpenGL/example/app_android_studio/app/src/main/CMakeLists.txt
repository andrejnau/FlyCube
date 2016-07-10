cmake_minimum_required (VERSION 3.4.1)

set(ANDROID_STL c++_shared)

set(PROJECT FlyCube)
project(${PROJECT})
set(target gles3jni)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(project_root ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../..)

option(BUILD_SHARED_LIBS "Build package with shared libraries." OFF)

add_definitions(-DASSETS_PATH="")

include(${project_root}/cmake/3rdparty/glm.cmake)
#include(${project_root}/cmake/3rdparty/assimp.cmake)
#include(${project_root}/cmake/3rdparty/soil.cmake)

add_subdirectory(${project_root}/OpenGL/common common)
#add_subdirectory(${project_root}/OpenGL/src/scene scene)

set(sources src/main.cpp)
add_library(${target} SHARED ${sources})
target_link_libraries(${target} android log GLESv3 EGL utils testscene)