set(target SponzaPbr)

function(gen_shaders_ref shaders shaders_ref)
    set(template ${project_root}/src/Apps/ParseShader/templates/program.in)
    set(output_dir ${CMAKE_BINARY_DIR}/gen/${target}/ProgramRef)
    foreach(full_shader_path ${shaders})
        get_filename_component(shader_name ${full_shader_path} NAME_WE)
        string(REPLACE "_" "" shader_name ${shader_name})
        string(REPLACE "${assets_path}" "" shader_path ${full_shader_path})
        set(output_file ${output_dir}/${shader_name}.h)
        get_property(entrypoint SOURCE ${full_shader_path} PROPERTY VS_SHADER_ENTRYPOINT)
        get_property(type SOURCE ${full_shader_path} PROPERTY VS_SHADER_TYPE)
        get_property(model SOURCE ${full_shader_path} PROPERTY VS_SHADER_MODEL)
        add_custom_command(OUTPUT ${output_file}
            COMMAND ${CMAKE_COMMAND} -E echo ${shader_name} ${shader_path} ${entrypoint} ${type} ${model} ${template} ${output_dir}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${output_dir}
            COMMAND $<TARGET_FILE:ParseShader> ${shader_name} ${shader_path} ${entrypoint} ${type} ${model} ${template} ${output_dir}
            DEPENDS ${template} "${full_shader_path}" ParseShader
        )
        set(output_shaders_ref ${output_shaders_ref} ${output_file})
    endforeach()
    set(${shaders_ref} ${output_shaders_ref} PARENT_SCOPE)
endfunction()

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}")
set(source_path "${CMAKE_CURRENT_SOURCE_DIR}")
set(shaders_path "${assets_path}/shaders/SponzaPbr")

set(headers
    ${include_path}/GeometryPass.h
    ${include_path}/LightPass.h
    ${include_path}/ImGuiPass.h
    ${include_path}/SSAOPass.h
    ${include_path}/IrradianceConversion.h
    ${include_path}/BackgroundPass.h
    ${include_path}/ComputeLuminance.h
    ${include_path}/Scene.h
    ${include_path}/ImGuiSettings.h
    ${include_path}/Settings.h
)

set(sources
    ${source_path}/GeometryPass.cpp
    ${source_path}/LightPass.cpp
    ${source_path}/ImGuiPass.cpp
    ${source_path}/SSAOPass.cpp
    ${source_path}/IrradianceConversion.cpp
    ${source_path}/BackgroundPass.cpp
    ${source_path}/ComputeLuminance.cpp
    ${source_path}/Scene.cpp
    ${source_path}/Settings.cpp
    ${source_path}/Main.cpp
)

set(pixel_shaders
    ${shaders_path}/GeometryPass_PS.hlsl
    ${shaders_path}/LightPass_PS.hlsl
    ${shaders_path}/ImGuiPass_PS.hlsl
    ${shaders_path}/HDRApply_PS.hlsl
    ${shaders_path}/SSAOPass_PS.hlsl
    ${shaders_path}/SSAOBlurPass_PS.hlsl
    ${shaders_path}/Equirectangular2Cubemap_PS.hlsl
    ${shaders_path}/IrradianceConvolution_PS.hlsl
    ${shaders_path}/Background_PS.hlsl
    ${shaders_path}/Prefilter_PS.hlsl
)

set(vertex_shaders
    ${shaders_path}/GeometryPass_VS.hlsl
    ${shaders_path}/LightPass_VS.hlsl
    ${shaders_path}/ImGuiPass_VS.hlsl
    ${shaders_path}/HDRApply_VS.hlsl
    ${shaders_path}/SSAOPass_VS.hlsl
    ${shaders_path}/Cubemap_VS.hlsl
    ${shaders_path}/Background_VS.hlsl
)

set(geometry_shaders
)

set(compute_shaders
    ${shaders_path}/HDRLum1DPass_CS.hlsl
    ${shaders_path}/HDRLum2DPass_CS.hlsl
)

set_property(SOURCE ${pixel_shaders} PROPERTY VS_SHADER_ENTRYPOINT main)
set_property(SOURCE ${pixel_shaders} PROPERTY VS_SHADER_TYPE Pixel)
set_property(SOURCE ${pixel_shaders} PROPERTY VS_SHADER_MODEL 5.0)

set_property(SOURCE ${vertex_shaders} PROPERTY VS_SHADER_ENTRYPOINT main)
set_property(SOURCE ${vertex_shaders} PROPERTY VS_SHADER_TYPE Vertex)
set_property(SOURCE ${vertex_shaders} PROPERTY VS_SHADER_MODEL 5.0)

set_property(SOURCE ${geometry_shaders} PROPERTY VS_SHADER_ENTRYPOINT main)
set_property(SOURCE ${geometry_shaders} PROPERTY VS_SHADER_TYPE Geometry)
set_property(SOURCE ${geometry_shaders} PROPERTY VS_SHADER_MODEL 5.0)

set_property(SOURCE ${compute_shaders} PROPERTY VS_SHADER_ENTRYPOINT main)
set_property(SOURCE ${compute_shaders} PROPERTY VS_SHADER_TYPE Compute)
set_property(SOURCE ${compute_shaders} PROPERTY VS_SHADER_MODEL 5.0)

set(shaders_files ${pixel_shaders} ${vertex_shaders} ${geometry_shaders} ${compute_shaders})

gen_shaders_ref("${shaders_files}" shaders_ref)

source_group("Shader Files" FILES ${shaders_files})
source_group("Shader Ref Files" FILES ${shaders_ref})

add_executable(${target} WIN32 ${headers} ${sources} ${shaders_files} ${shaders_ref})
set_target_properties(${target} PROPERTIES
                        LINK_FLAGS "/ENTRY:mainCRTStartup")

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        ${CMAKE_BINARY_DIR}/gen/${target}
)

target_link_libraries(${target}
    Context
    d3dcompiler
    dxgi
    d3d11
    dxguid
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
