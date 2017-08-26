set(target DX11App)

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(source_path "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(shaders_path "${assets_path}/shaders/DX11")

set(headers
    ${include_path}/DXSample.h
    ${include_path}/DX11Geometry.h
    ${include_path}/Shaders.h
)

set(sources
    ${source_path}/DXSample.cpp
    ${source_path}/Main.cpp
)

set(pixel_shaders
    ${shaders_path}/PixelShader.hlsl
    ${shaders_path}/GeometryPass_PS.hlsl
    ${shaders_path}/LightPass_PS.hlsl
)

set(vertex_shaders
    ${shaders_path}/VertexShader.hlsl
    ${shaders_path}/GeometryPass_VS.hlsl
    ${shaders_path}/LightPass_VS.hlsl
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
set_target_properties(${target} PROPERTIES
                        LINK_FLAGS "/ENTRY:mainCRTStartup")

target_compile_definitions(${target} PRIVATE UNICODE _UNICODE NOMINMAX)

target_link_libraries(${target} d3dcompiler dxgi d3d11 assimp SOIL D3DUtils AppBox)

target_include_directories(${target}
    PUBLIC
        ${project_root}/Source
)
