set(target DX11App)

function(add_shader shader_name shader_path entrypoint target)
    set(template ${project_root}/src/Apps/ParseShader/templates/program.in)
    set(output_dir ${CMAKE_BINARY_DIR}/gen/ProgramRef)
    set(output_file ${output_dir}/${shader_name}.h)
    set(program_reflection ${program_reflection} ${output_file} PARENT_SCOPE)
    add_custom_command(OUTPUT ${output_file}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${output_dir}
        COMMAND $<TARGET_FILE:ParseShader> ${shader_name} ${shader_path} ${entrypoint} ${target} ${template} ${output_dir}
        DEPENDS ${template} "${assets_path}/${shader_path}"
    )
    source_group("Shader Ref Files" FILES ${output_file})
endfunction()

add_shader(GeometryPassPS "shaders/DX11/GeometryPass_PS.hlsl" main ps_5_0)
add_shader(GeometryPassVS "shaders/DX11/GeometryPass_VS.hlsl" main vs_5_0)
add_shader(LightPassPS "shaders/DX11/LightPass_PS.hlsl" main ps_5_0)
add_shader(LightPassVS "shaders/DX11/LightPass_VS.hlsl" main vs_5_0)
add_shader(ImGuiPassPS "shaders/DX11/ImGuiPass_PS.hlsl" main ps_5_0)
add_shader(ImGuiPassVS "shaders/DX11/ImGuiPass_VS.hlsl" main vs_5_0)

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}")
set(source_path "${CMAKE_CURRENT_SOURCE_DIR}")
set(shaders_path "${assets_path}/shaders/DX11")

set(headers
    ${source_path}/GeometryPass.h
    ${source_path}/LightPass.h
    ${source_path}/ImGuiPass.h
    ${include_path}/DX11Scene.h
)

set(sources
    ${source_path}/GeometryPass.cpp
    ${source_path}/LightPass.cpp
    ${source_path}/ImGuiPass.cpp
    ${source_path}/DX11Scene.cpp
    ${source_path}/Main.cpp
)

set(pixel_shaders
    ${shaders_path}/GeometryPass_PS.hlsl
    ${shaders_path}/LightPass_PS.hlsl
    ${shaders_path}/ImGuiPass_PS.hlsl
)

set(vertex_shaders
    ${shaders_path}/GeometryPass_VS.hlsl
    ${shaders_path}/LightPass_VS.hlsl
    ${shaders_path}/ImGuiPass_VS.hlsl
)

set_property(SOURCE ${pixel_shaders} PROPERTY VS_SHADER_TYPE Pixel)
set_property(SOURCE ${pixel_shaders} PROPERTY VS_SHADER_ENTRYPOINT main)
set_property(SOURCE ${pixel_shaders} PROPERTY VS_SHADER_MODEL 5.0)

set_property(SOURCE ${vertex_shaders} PROPERTY VS_SHADER_TYPE Vertex)
set_property(SOURCE ${vertex_shaders} PROPERTY VS_SHADER_ENTRYPOINT main)
set_property(SOURCE ${vertex_shaders} PROPERTY VS_SHADER_MODEL 5.0)

set(resource_files ${pixel_shaders} ${vertex_shaders})

source_group("Shader Files" FILES ${resource_files})

add_executable(${target} WIN32 ${headers} ${sources} ${resource_files} ${program_reflection})
set_target_properties(${target} PROPERTIES
                        LINK_FLAGS "/ENTRY:mainCRTStartup")

target_compile_definitions(${target} PRIVATE UNICODE _UNICODE NOMINMAX)

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        ${CMAKE_BINARY_DIR}/gen
)

target_link_libraries(${target}
    Context
    d3dcompiler
    dxgi
    d3d11
    assimp
    SOIL
    AppBox
    Scene
    Geometry
    Utilities
    Texture
    Program
    imgui
)
