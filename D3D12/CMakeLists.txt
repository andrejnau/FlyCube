cmake_minimum_required(VERSION 3.5)

set (PROJECT DX12)
project (${PROJECT})

#compiler settings
include(../cmake/defaults.cmake)

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

set(pixel_shaders
    ${source_path}/shaders/PixelShader.hlsl
)

set(vertex_shaders
    ${source_path}/shaders/VertexShader.hlsl
)

set_property(SOURCE ${pixel_shaders} PROPERTY VS_SHADER_TYPE Pixel)
set_property(SOURCE ${pixel_shaders} PROPERTY VS_SHADER_ENTRYPOINT main)
set_property(SOURCE ${pixel_shaders} PROPERTY VS_SHADER_MODEL 5.0)

set_property(SOURCE ${vertex_shaders} PROPERTY VS_SHADER_TYPE Vertex)
set_property(SOURCE ${vertex_shaders} PROPERTY VS_SHADER_ENTRYPOINT main)
set_property(SOURCE ${vertex_shaders} PROPERTY VS_SHADER_MODEL 5.0)

set(resource_files ${pixel_shaders} ${vertex_shaders})

source_group("Заголовочные файлы" FILES ${headers})
source_group("Файлы исходного кода" FILES ${sources})
source_group("Файлы ресурсов" FILES ${resource_files})

add_executable(${PROJECT} WIN32 ${headers} ${sources} ${resource_files})

add_definitions(-DASSETS_PATH=L"${CMAKE_CURRENT_SOURCE_DIR}/src/")

target_link_libraries(${PROJECT} d3dx12 d3dcompiler dxgi d3d12)