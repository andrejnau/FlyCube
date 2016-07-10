cmake_minimum_required(VERSION 3.5)

set(target DX12)

include(${project_root}/cmake/3rdparty/soil.cmake)
include(${project_root}/cmake/3rdparty/assimp.cmake)

add_subdirectory(3rdparty/d3dx12)

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(source_path "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(shaders_path "${assets_path}/shaders/DX12")

set(headers
    ${include_path}/DXSample.h
    ${include_path}/IDXSample.h
    ${include_path}/Util.h
    ${include_path}/Utility.h
    ${include_path}/Geometry.h
    ${include_path}/FileUtility.h
    ${include_path}/Win32Application.h
)

set(sources
    ${source_path}/DXSample.cpp
    ${source_path}/Geometry.cpp
    ${source_path}/Util.cpp
    ${source_path}/Main.cpp
    ${source_path}/Win32Application.cpp
)

set(pixel_shaders
    ${shaders_path}/PixelShader.hlsl
)

set(vertex_shaders
    ${shaders_path}/VertexShader.hlsl
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

add_executable(${target} WIN32 ${headers} ${sources} ${resource_files})

target_compile_definitions(${target} PRIVATE UNICODE _UNICODE NOMINMAX)

target_link_libraries(${target} d3dx12 d3dcompiler dxgi d3d12 assimp SOIL)