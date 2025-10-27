function(copy_assets base_dir output_subdir files output_var)
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
    unset(copy_tasks)
    foreach(full_file_path ${files})
        cmake_path(RELATIVE_PATH full_file_path BASE_DIRECTORY "${base_dir}" OUTPUT_VARIABLE file_name)
        set(output_file "${output_dir}/${file_name}")
        set(gen_dir "${PROJECT_BINARY_DIR}/copy_tasks/${output_subdir}")
        set(gen_file "${gen_dir}/${file_name}.copy_task")
        add_custom_command(OUTPUT "${gen_file}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${output_dir}"
            COMMAND ${CMAKE_COMMAND} -E copy "${full_file_path}" "${output_file}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${gen_dir}"
            COMMAND ${CMAKE_COMMAND} -E touch "${gen_file}"
            DEPENDS "${full_file_path}"
        )
        list(APPEND copy_tasks
            "${gen_file}"
        )
    endforeach()
    set_source_files_properties(${files} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources/${output_subdir}"
    )
    source_group("Assets Files" FILES ${files})
    source_group("Copy Tasks" FILES ${copy_tasks})
    set(${output_var} ${${output_var}} ${files} ${copy_tasks} PARENT_SCOPE)
endfunction()
