function(compile_shaders base_dir output_subdir shaders output_var)
    if (CMAKE_CROSSCOMPILING AND IOS_OR_TVOS)
        set(shader_compiler_cli "${CMAKE_BINARY_DIR}/macOS/bin/ShaderCompilerCLI")
    else()
        set(shader_compiler_cli "$<TARGET_FILE:ShaderCompilerCLI>")
    endif()
    if (CMAKE_CONFIGURATION_TYPES)
        set(output_dir "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${output_subdir}/")
    else()
        set(output_dir "${CMAKE_BINARY_DIR}/bin/${output_subdir}/")
    endif()
    set(gen_dir "${CMAKE_BINARY_DIR}/compiled_shaders/${output_subdir}/")
    unset(compiled_shaders)
    foreach(full_shader_path ${shaders})
        cmake_path(RELATIVE_PATH full_shader_path BASE_DIRECTORY "${base_dir}" OUTPUT_VARIABLE shader_name)
        get_filename_component(shader_folder ${shader_name} DIRECTORY)
        set(spirv ${gen_dir}/${shader_name}.spirv)
        set(dxil ${gen_dir}/${shader_name}.dxil)
        get_property(entrypoint SOURCE ${full_shader_path} PROPERTY SHADER_ENTRYPOINT)
        get_property(type SOURCE ${full_shader_path} PROPERTY SHADER_TYPE)
        get_property(model SOURCE ${full_shader_path} PROPERTY SHADER_MODEL)
        add_custom_command(OUTPUT ${spirv} ${dxil}
            COMMAND ${CMAKE_COMMAND} -E echo ${shader_name} ${full_shader_path} ${entrypoint} ${type} ${model} ${gen_dir}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${gen_dir}/${shader_folder}
            COMMAND ${shader_compiler_cli} ${shader_name} ${full_shader_path} ${entrypoint} ${type} ${model} ${gen_dir}
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${gen_dir}/${shader_folder} ${output_dir}/${shader_folder}
            DEPENDS ShaderCompilerCLI
            MAIN_DEPENDENCY "${full_shader_path}"
        )
        set_source_files_properties(${spirv} ${dxil} PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources/${output_subdir}/${shader_folder}"
        )
        source_group("Shader Files" FILES "${full_shader_path}")
        source_group("Shader Blobs" FILES ${spirv} ${dxil})
        set(compiled_shaders ${compiled_shaders} ${spirv})
        set(compiled_shaders ${compiled_shaders} ${dxil})
    endforeach()
    set(${output_var} ${compiled_shaders} PARENT_SCOPE)
endfunction()
