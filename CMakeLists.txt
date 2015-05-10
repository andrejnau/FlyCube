cmake_minimum_required (VERSION 3.0)

set (PROJECT FlyCube)
project (${PROJECT})

include(cmake/defaults.cmake)

#OpenGL
find_package(OpenGL REQUIRED)

#qt
find_package(Qt5Core)
find_package(Qt5Gui)
find_package(Qt5Widgets)

#glfw
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")
add_subdirectory(3rdparty/glfw-3.1.1)
set_target_properties(glfw PROPERTIES FOLDER "3rdparty")
include_directories(3rdparty/glfw-3.1.1/include)
add_definitions("-DGLFW_INCLUDE_NONE")

#Simple-OpenGL-Image-Library
add_subdirectory(3rdparty/Simple-OpenGL-Image-Library-master)
include_directories(3rdparty/Simple-OpenGL-Image-Library-master/src)
set_target_properties(SOIL PROPERTIES FOLDER "3rdparty")

#glm
include_directories(3rdparty/glm)

#gl_core
add_subdirectory(3rdparty/gl_core)
include_directories(3rdparty/gl_core)
set_target_properties(gl_core PROPERTIES FOLDER "3rdparty")

#project
add_definitions(-DPROJECT_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources/")

include_directories(common/utils/include)
include_directories(common/testscene/include)
include_directories(common/mlogger/include)
include_directories(common/camera/include)

add_subdirectory(common)
add_subdirectory(example)
add_subdirectory(learning_tasks)