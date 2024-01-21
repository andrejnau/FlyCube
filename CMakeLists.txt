cmake_minimum_required(VERSION 3.22)
project(FlyCube)

if (CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "tvOS")
    set(IOS_OR_TVOS TRUE)
endif()

include(CMakeDependentOption)
option(STATIC_MOLTEN_VK "Link with MoltenVK.xcframework on macOS/iOS/tvOS" OFF)
option(VULKAN_SUPPORT "Vulkan support" ON)
cmake_dependent_option(DIRECTX_SUPPORT "DirectX 12 support" ON "WIN32" OFF)
option(AGILITY_SDK_REQUIRED "Use Agility SDK" OFF)
cmake_dependent_option(METAL_SUPPORT "Metal support" ON "APPLE" OFF)
cmake_dependent_option(BUILD_SAMPLES "Build samples" ON "NOT ANDROID" OFF)
cmake_dependent_option(BUILD_TESTING "Build unit tests" ON "NOT IOS_OR_TVOS" OFF)

set(project_root "${CMAKE_CURRENT_SOURCE_DIR}")
set(assets_path "${project_root}/assets/")
add_library(FlyCubeAssets INTERFACE)
target_compile_definitions(FlyCubeAssets INTERFACE ASSETS_PATH="${assets_path}")

list(PREPEND CMAKE_MODULE_PATH "${project_root}/cmake")
include(codesign_settings_apple OPTIONAL)
include(cmake_settings)
include(compiler_settings)

include(3rdparty/glfw)
include(3rdparty/glm)
include(3rdparty/gli)
include(3rdparty/mvkpixelformats)
include(3rdparty/nowide)
include(3rdparty/catch2)
include(3rdparty/dxc)
include(3rdparty/directx)
include(3rdparty/vulkan)
include(3rdparty/spirv-cross)

if (VULKAN_SUPPORT)
    add_definitions(-DVULKAN_SUPPORT)
endif()
if (DIRECTX_SUPPORT)
    add_definitions(-DDIRECTX_SUPPORT)
endif()
if (AGILITY_SDK_REQUIRED)
    add_definitions(-DAGILITY_SDK_REQUIRED)
endif()
if (METAL_SUPPORT)
    add_definitions(-DMETAL_SUPPORT)
endif()

if (BUILD_TESTING)
    enable_testing()
endif()

add_subdirectory(src)

if (CMAKE_CROSSCOMPILING AND IOS_OR_TVOS AND BUILD_SAMPLES)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" "${CMAKE_CURRENT_SOURCE_DIR}" -BmacOS
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --build macOS --config Release --target ShaderCompilerCLI -j 16
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()
