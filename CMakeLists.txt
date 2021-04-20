cmake_minimum_required(VERSION 3.15)
project(FlyCube)

set(project_root "${CMAKE_CURRENT_SOURCE_DIR}")
set(assets_path "${project_root}/assets/")
add_definitions(-DASSETS_PATH="${assets_path}")

list(PREPEND CMAKE_MODULE_PATH "${project_root}/cmake")
include(cmake_settings)
include(compiler_settings)

set(VULKAN_SUPPORT ON)
if (WIN32)
    set(DIRECTX_SUPPORT ON)
endif()

include(3rdparty/glfw)
include(3rdparty/soil)
include(3rdparty/assimp)
include(3rdparty/glm)
include(3rdparty/gli)
include(3rdparty/mustache)
include(3rdparty/catch2)
include(3rdparty/dxc)

if (VULKAN_SUPPORT)
    add_definitions(-DVULKAN_SUPPORT)
    include(3rdparty/vulkan)
    include(3rdparty/spirv-cross)
endif()

if (DIRECTX_SUPPORT)
    add_definitions(-DDIRECTX_SUPPORT)
    include(3rdparty/directx)
endif()

add_subdirectory(src)
