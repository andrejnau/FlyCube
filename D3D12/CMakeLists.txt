cmake_minimum_required(VERSION 3.5)

set (PROJECT DX12_CMAKE)
project (${PROJECT})

add_subdirectory(3rdparty/d3dx12)

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(source_path "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_definitions(-D_UNICODE)

set(headers
    ${include_path}/DXSample.h
    ${include_path}/IDXSample.h
    ${include_path}/Utility.h
    ${include_path}/FileUtility.h
    ${include_path}/Win32Application.h
)

set(sources
    ${source_path}/DXSample.cpp
    ${source_path}/Main.cpp
    ${source_path}/Win32Application.cpp
)

add_executable(${PROJECT} WIN32 ${headers} ${sources})

add_definitions(-DASSETS_PATH=L"${CMAKE_CURRENT_SOURCE_DIR}/src/")

target_link_libraries(${PROJECT} d3dx12 d3dcompiler dxgi d3d12)