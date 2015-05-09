cmake_minimum_required (VERSION 3.0)

set (PROJECT FlyCube)
project (${PROJECT})

include(cmake/defaults.cmake)

#qt
find_package(Qt5Core)
find_package(Qt5Gui)
find_package(Qt5Widgets)

if (Qt5Core_FOUND)
    set(CMAKE_AUTOMOC ON)
    set(AUTOMOC_MOC_OPTIONS PROPERTIES FOLDER CMakeAutomocTargets)
    #set_property(GLOBAL PROPERTY AUTOMOC_FOLDER CMakeAutomocTargets)
else()
    message(WARNING "Qt framework not found")
endif()

#glfw
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")
add_subdirectory(3rdparty/glfw-3.1)
set_target_properties(glfw PROPERTIES FOLDER "3rdparty")
include_directories(3rdparty/glfw-3.1/include)
add_definitions("-DGLFW_INCLUDE_NONE")

#Simple-OpenGL-Image-Library
add_subdirectory(3rdparty/Simple-OpenGL-Image-Library-master)
include_directories(3rdparty/Simple-OpenGL-Image-Library-master/src)
set_target_properties(SOIL PROPERTIES FOLDER "3rdparty")

#glm
include_directories(3rdparty/glm)

#camera
add_subdirectory(3rdparty/moderngl_camera-master)
include_directories(3rdparty/moderngl_camera-master)

#gl_core
add_subdirectory(3rdparty/gl_core)
include_directories(3rdparty/gl_core)
set_target_properties(gl_core PROPERTIES FOLDER "3rdparty")

#project
add_definitions(-DPROJECT_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/core_source/res/")
add_definitions(-DPROJECT_RESOURCE_MODEL_DIR="${CMAKE_CURRENT_SOURCE_DIR}/core_source/res/obj-model/")

include_directories(common/mlogger/include)

add_subdirectory(common)
add_subdirectory(core_source)
add_subdirectory(example)
add_subdirectory(learning_tasks)