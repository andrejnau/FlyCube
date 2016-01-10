cmake_minimum_required (VERSION 3.0)

set (PROJECT FlyCube)
project (${PROJECT})

set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

#compiler settings
include(cmake/defaults.cmake)

#init 3rdparty library
include(cmake/3rdparty/qt.cmake)
include(cmake/3rdparty/glfw.cmake)
include(cmake/3rdparty/soil.cmake)
include(cmake/3rdparty/assimp.cmake)
include(cmake/3rdparty/glm.cmake)
include(cmake/3rdparty/glbinding.cmake)

#project
add_definitions(-DPROJECT_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources/")

add_subdirectory(common)
add_subdirectory(example)
add_subdirectory(samples)