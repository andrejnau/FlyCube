cmake_minimum_required (VERSION 3.2.2)

set (PROJECT FlyCube)
project (${PROJECT})

option(BUILD_OPENGL "Build OpenGL sample" OFF)
option(BUILD_DX11   "Build DX11 sample"   ON)
option(BUILD_DX12   "Build DX12 sample"   ON)

set(project_root ${CMAKE_CURRENT_SOURCE_DIR})
set(assets_path ${CMAKE_CURRENT_SOURCE_DIR}/assets/)

#compiler settings
include(${project_root}/cmake/defaults.cmake)

add_definitions(-DASSETS_PATH="${assets_path}")

include(${project_root}/cmake/3rdparty/soil.cmake)
include(${project_root}/cmake/3rdparty/assimp.cmake)
include(${project_root}/cmake/3rdparty/SimpleMath.cmake)

add_subdirectory(D3DUtils)

if (BUILD_OPENGL)
    add_subdirectory(OpenGL)
endif()

if(BUILD_DX11)
    add_subdirectory(D3D11)
endif()

if(BUILD_DX12)
    add_subdirectory(D3D12)
endif()
