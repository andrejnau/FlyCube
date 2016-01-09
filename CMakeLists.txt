cmake_minimum_required (VERSION 3.0)

set (PROJECT FlyCube)
project (${PROJECT})

set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

#compiler settings
include(cmake/defaults.cmake)

#init 3rdparty library
include(cmake/3rdparty/opengl.cmake)
include(cmake/3rdparty/qt.cmake)
include(cmake/3rdparty/glfw.cmake)
include(cmake/3rdparty/soil.cmake)
include(cmake/3rdparty/assimp.cmake)
include(cmake/3rdparty/glm.cmake)
include(cmake/3rdparty/glad.cmake)

#project
add_definitions(-DPROJECT_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources/")

include_directories(common/utils/include)
include_directories(common/testscene/include)
include_directories(common/mlogger/include)
include_directories(common/simple_camera/include)

add_subdirectory(common)
add_subdirectory(example)
add_subdirectory(samples)