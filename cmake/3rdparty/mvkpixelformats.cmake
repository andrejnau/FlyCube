if (NOT METAL_SUPPORT)
    return()
endif()

add_subdirectory("${project_root}/3rdparty/MVKPixelFormats")
set_target_properties(MVKPixelFormats PROPERTIES FOLDER "3rdparty/MVKPixelFormats")
