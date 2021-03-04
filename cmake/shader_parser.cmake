function(gen_shaders_ref target shaders shaders_ref)
    set(build_folder $<TARGET_FILE_DIR:${target}>)
    set(template "$<TARGET_PROPERTY:ShaderParser,SOURCE_DIR>/templates/program.in")
    set(output_dir "${CMAKE_BINARY_DIR}/gen/${target}/ProgramRef")
    foreach(full_shader_path ${shaders})
        get_filename_component(shader_name ${full_shader_path} NAME_WE)
        string(REPLACE "_" "" shader_name ${shader_name})
        string(REPLACE "${assets_path}" "" shader_path ${full_shader_path})
        set(output_file ${output_dir}/${shader_name}.h)
        get_property(entrypoint SOURCE ${full_shader_path} PROPERTY VS_SHADER_ENTRYPOINT)
        get_property(type SOURCE ${full_shader_path} PROPERTY VS_SHADER_TYPE)
        get_property(model SOURCE ${full_shader_path} PROPERTY VS_SHADER_MODEL)
        add_custom_command(OUTPUT ${output_file}
            COMMAND ${CMAKE_COMMAND} -E echo ${assets_path} ${shader_name} ${shader_path} ${entrypoint} ${type} ${model} ${template} ${output_dir} ${build_folder}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${output_dir}
            COMMAND $<TARGET_FILE:ShaderParser> ${assets_path} ${shader_name} ${shader_path} ${entrypoint} ${type} ${model} ${template} ${output_dir} ${build_folder}
            DEPENDS ${template} ShaderParser
            MAIN_DEPENDENCY "${full_shader_path}"
        )
        set(output_shaders_ref ${output_shaders_ref} ${output_file})
    endforeach()
    set(${shaders_ref} ${output_shaders_ref} PARENT_SCOPE)
endfunction()
