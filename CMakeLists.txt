cmake_minimum_required (VERSION 3.0.2)

set (PROJECT FlyCube)
project (${PROJECT})

set(project_root ${CMAKE_CURRENT_SOURCE_DIR})
set(assets_path ${CMAKE_CURRENT_SOURCE_DIR}/assets/)

#compiler settings
include(${project_root}/cmake/defaults.cmake)

add_definitions(-DASSETS_PATH="${assets_path}")

if(D3D12)
    add_subdirectory(D3D12)
else()
    add_subdirectory(OpenGL)
endif()