function(compile_shaders target shaders output)
    if (CMAKE_CROSSCOMPILING AND IOS_OR_TVOS)
        set(shader_compiler_cli "${CMAKE_BINARY_DIR}/macOS/bin/ShaderCompilerCLI")
    else()
        set(shader_compiler_cli "$<TARGET_FILE:ShaderCompilerCLI>")
    endif()
    set(output_dir "${CMAKE_BINARY_DIR}/gen/${target}/")
    foreach(full_shader_path ${shaders})
        get_filename_component(shader_name ${full_shader_path} NAME_WE)
        set(spirv ${output_dir}/${shader_name}.spirv)
        set(dxil ${output_dir}/${shader_name}.dxil)
        get_property(entrypoint SOURCE ${full_shader_path} PROPERTY SHADER_ENTRYPOINT)
        get_property(type SOURCE ${full_shader_path} PROPERTY SHADER_TYPE)
        get_property(model SOURCE ${full_shader_path} PROPERTY SHADER_MODEL)
        add_custom_command(OUTPUT ${spirv} ${dxil}
            COMMAND ${CMAKE_COMMAND} -E echo ${shader_name} ${full_shader_path} ${entrypoint} ${type} ${model} ${output_dir}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${output_dir}
            COMMAND ${shader_compiler_cli} ${shader_name} ${full_shader_path} ${entrypoint} ${type} ${model} ${output_dir}
            DEPENDS ShaderCompilerCLI
            MAIN_DEPENDENCY "${full_shader_path}"
        )
        set(compiled_shaders ${compiled_shaders} ${spirv})
        set(compiled_shaders ${compiled_shaders} ${dxil})
    endforeach()
    set(${output} ${compiled_shaders} PARENT_SCOPE)
endfunction()
