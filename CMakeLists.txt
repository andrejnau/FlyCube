cmake_minimum_required (VERSION 3.2.2)

set (PROJECT FlyCube)
project (${PROJECT})

set(project_root ${CMAKE_CURRENT_SOURCE_DIR})
set(assets_path ${CMAKE_CURRENT_SOURCE_DIR}/assets/)

#compiler settings
include(${project_root}/cmake/defaults.cmake)

add_definitions(-DASSETS_PATH="${assets_path}")

include(${project_root}/cmake/3rdparty/glfw.cmake)
include(${project_root}/cmake/3rdparty/soil.cmake)
include(${project_root}/cmake/3rdparty/assimp.cmake)
include(${project_root}/cmake/3rdparty/glm.cmake)
include(${project_root}/cmake/3rdparty/gli.cmake)
include(${project_root}/cmake/3rdparty/gl.cmake)
include(${project_root}/cmake/3rdparty/mustache.cmake)
include(${project_root}/cmake/3rdparty/imgui.cmake)
include(${project_root}/cmake/3rdparty/spirv-cross.cmake)
include(${project_root}/cmake/3rdparty/vulkan.cmake)
include(${project_root}/cmake/3rdparty/pix3.cmake)

add_subdirectory(src)
