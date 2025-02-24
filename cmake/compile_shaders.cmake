function(compile_shaders base_dir output_subdir shaders output_var)
    if (ANDROID)
        set(output_dir_pref "${ANDROID_ASSETS_DIR}/")
    else()
        set(output_dir_pref "${PROJECT_BINARY_DIR}/bin/")
    endif()
    if (CMAKE_CONFIGURATION_TYPES)
        set(output_dir "${output_dir_pref}/$<CONFIG>/${output_subdir}/")
    else()
        set(output_dir "${output_dir_pref}/${output_subdir}/")
    endif()
    set(gen_dir "${PROJECT_BINARY_DIR}/compiled_shaders/${output_subdir}/")
    unset(compiled_shaders)
    foreach(full_shader_path ${shaders})
        cmake_path(RELATIVE_PATH full_shader_path BASE_DIRECTORY "${base_dir}" OUTPUT_VARIABLE shader_name)
        set(spirv ${gen_dir}/${shader_name}.spirv)
        set(dxil ${gen_dir}/${shader_name}.dxil)
        get_property(entrypoint SOURCE ${full_shader_path} PROPERTY SHADER_ENTRYPOINT)
        get_property(type SOURCE ${full_shader_path} PROPERTY SHADER_TYPE)
        get_property(model SOURCE ${full_shader_path} PROPERTY SHADER_MODEL)
        add_custom_command(OUTPUT ${spirv} ${dxil}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${gen_dir}
            COMMAND ${CMAKE_COMMAND} -E echo $<TARGET_FILE:ShaderCompilerCLI> ${shader_name} ${full_shader_path} ${entrypoint} ${type} ${model} ${gen_dir}
            COMMAND $<TARGET_FILE:ShaderCompilerCLI> ${shader_name} ${full_shader_path} ${entrypoint} ${type} ${model} ${gen_dir}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${output_dir}
            COMMAND ${CMAKE_COMMAND} -E copy ${spirv} ${output_dir}/${shader_name}.spirv
            COMMAND ${CMAKE_COMMAND} -E copy ${dxil} ${output_dir}/${shader_name}.dxil
            DEPENDS ShaderCompilerCLI
            MAIN_DEPENDENCY "${full_shader_path}"
        )
        set_source_files_properties(${spirv} ${dxil} PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources/${output_subdir}"
        )
        source_group("Shader Files" FILES "${full_shader_path}")
        source_group("Shader Blobs" FILES ${spirv} ${dxil})
        set(compiled_shaders ${compiled_shaders} ${spirv})
        set(compiled_shaders ${compiled_shaders} ${dxil})
    endforeach()
    set(${output_var} ${compiled_shaders} PARENT_SCOPE)
endfunction()
