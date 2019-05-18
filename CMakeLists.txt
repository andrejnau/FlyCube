cmake_minimum_required (VERSION 3.2.2)

set (PROJECT FlyCube)
project (${PROJECT})

set(project_root ${CMAKE_CURRENT_SOURCE_DIR})
set(assets_path ${CMAKE_CURRENT_SOURCE_DIR}/assets/)
set(sdkbin_path "C:/Program Files (x86)/Windows Kits/10/Bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")

#compiler settings
include(${project_root}/cmake/defaults.cmake)

add_definitions(-DASSETS_PATH="${assets_path}")
add_definitions(-DSDKBIN="${sdkbin_path}")

set(VULKAN_SUPPORT "ON")

include(${project_root}/cmake/3rdparty/glfw.cmake)
include(${project_root}/cmake/3rdparty/soil.cmake)
include(${project_root}/cmake/3rdparty/assimp.cmake)
include(${project_root}/cmake/3rdparty/glm.cmake)
include(${project_root}/cmake/3rdparty/gli.cmake)
include(${project_root}/cmake/3rdparty/gl.cmake)
include(${project_root}/cmake/3rdparty/mustache.cmake)
include(${project_root}/cmake/3rdparty/imgui.cmake)
include(${project_root}/cmake/3rdparty/spirv-cross.cmake)
if (VULKAN_SUPPORT)
    include(${project_root}/cmake/3rdparty/vulkan.cmake)
    add_definitions(-DVULKAN_SUPPORT)
endif()
include(${project_root}/cmake/3rdparty/dxc.cmake)

add_subdirectory(src)
