set(target DxrTriangle)

function(gen_shaders_ref shaders shaders_ref)
    set(template ${project_root}/src/Apps/ParseShader/templates/program.in)
    set(output_dir ${CMAKE_BINARY_DIR}/gen/${target}/ProgramRef)
    foreach(full_shader_path ${shaders})
        get_filename_component(shader_name ${full_shader_path} NAME_WE)
        string(REPLACE "_" "" shader_name ${shader_name})
        string(REPLACE "${assets_path}" "" shader_path ${full_shader_path})
        set(output_file ${output_dir}/${shader_name}.h)
        get_property(entrypoint SOURCE ${full_shader_path} PROPERTY VS_SHADER_ENTRYPOINT)
        if ("${entrypoint}" STREQUAL " ")
            set(entrypoint "empty")
        endif()
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
set(shaders_path "${assets_path}/shaders/DxrTriangle")

set(sources
    ${source_path}/main.cpp
)

set(pixel_shaders
    ${shaders_path}/Graphics_PS.hlsl
)

set(vertex_shaders
    ${shaders_path}/Graphics_VS.hlsl
)

set(lib_shaders
    ${shaders_path}/RayTracing.hlsl
)

set_property(SOURCE ${lib_shaders} PROPERTY VS_SHADER_ENTRYPOINT " ")
set_property(SOURCE ${lib_shaders} PROPERTY VS_SHADER_TYPE Library)
set_property(SOURCE ${lib_shaders} PROPERTY VS_SHADER_MODEL 6.3)
set_property(SOURCE ${lib_shaders} PROPERTY VS_SHADER_FLAGS "/Zi")

set_property(SOURCE ${pixel_shaders} PROPERTY VS_SHADER_ENTRYPOINT main)
set_property(SOURCE ${pixel_shaders} PROPERTY VS_SHADER_TYPE Pixel)
set_property(SOURCE ${pixel_shaders} PROPERTY VS_SHADER_MODEL 6.3)
set_property(SOURCE ${pixel_shaders} PROPERTY VS_SHADER_FLAGS "/Zi")

set_property(SOURCE ${vertex_shaders} PROPERTY VS_SHADER_ENTRYPOINT main)
set_property(SOURCE ${vertex_shaders} PROPERTY VS_SHADER_TYPE Vertex)
set_property(SOURCE ${vertex_shaders} PROPERTY VS_SHADER_MODEL 6.3)
set_property(SOURCE ${vertex_shaders} PROPERTY VS_SHADER_FLAGS "/Zi")

set(shaders_files ${lib_shaders} ${pixel_shaders} ${vertex_shaders})

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
    assimp
    SOIL
    AppBox
    Geometry
    Utilities
    Texture
    Program
    imgui
)

set_target_properties(${target} PROPERTIES FOLDER "Apps")
