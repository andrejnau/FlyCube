set(target Triangle)

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}")
set(source_path "${CMAKE_CURRENT_SOURCE_DIR}")
set(shaders_path "${assets_path}/shaders/Triangle")

set(sources
    ${source_path}/main.cpp
)

set(pixel_shaders
    ${shaders_path}/PixelShader.hlsl
)

set(vertex_shaders
    ${shaders_path}/VertexShader.hlsl
)

set_property(SOURCE ${pixel_shaders} PROPERTY VS_SHADER_ENTRYPOINT main)
set_property(SOURCE ${pixel_shaders} PROPERTY VS_SHADER_TYPE Pixel)
set_property(SOURCE ${pixel_shaders} PROPERTY VS_SHADER_MODEL 6.0)
set_property(SOURCE ${pixel_shaders} PROPERTY VS_SHADER_FLAGS "/Zi /auto-binding-space 0")

set_property(SOURCE ${vertex_shaders} PROPERTY VS_SHADER_ENTRYPOINT main)
set_property(SOURCE ${vertex_shaders} PROPERTY VS_SHADER_TYPE Vertex)
set_property(SOURCE ${vertex_shaders} PROPERTY VS_SHADER_MODEL 6.0)
set_property(SOURCE ${vertex_shaders} PROPERTY VS_SHADER_FLAGS "/Zi /auto-binding-space 0")

set(shaders_files ${pixel_shaders} ${vertex_shaders})

include(shader_parser)
gen_shaders_ref(${target} "${shaders_files}" shaders_ref)

source_group("Shader Files" FILES ${shaders_files})
source_group("Shader Ref Files" FILES ${shaders_ref})

add_executable(${target} WIN32 ${headers} ${sources} ${shaders_files} ${shaders_ref})

if (WIN32)
    set_target_properties(${target} PROPERTIES
                            LINK_FLAGS "/ENTRY:mainCRTStartup")
endif()

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        ${CMAKE_BINARY_DIR}/gen/${target}
)

target_link_libraries(${target}
    AppBox
    RenderDevice
)

set_target_properties(${target} PROPERTIES FOLDER "Apps")

install(TARGETS ${target})
