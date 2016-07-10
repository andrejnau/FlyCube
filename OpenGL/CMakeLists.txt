cmake_minimum_required (VERSION 3.0)

set (PROJECT FlyCube)
project (${PROJECT})

#init 3rdparty library
include(${project_root}/cmake/3rdparty/glfw.cmake)
include(${project_root}/cmake/3rdparty/soil.cmake)
include(${project_root}/cmake/3rdparty/assimp.cmake)
include(${project_root}/cmake/3rdparty/glm.cmake)

#project
add_definitions(-DPROJECT_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources/")

add_subdirectory(common)
add_subdirectory(common/gl_load)
add_subdirectory(example)
add_subdirectory(samples)