cmake_minimum_required (VERSION 3.5)

set (PROJECT FlyCube)
project (${PROJECT})

set(project_root ${CMAKE_CURRENT_SOURCE_DIR})

#compiler settings
include(${project_root}/cmake/defaults.cmake)

if(D3D12)
    add_subdirectory(D3D12)
else()
    add_subdirectory(OpenGL)
endif()