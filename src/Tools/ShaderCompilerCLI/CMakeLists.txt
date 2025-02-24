if (CMAKE_CROSSCOMPILING)
    set(shader_compiler_cli "${PROJECT_BINARY_DIR}/hostPlatform/bin/ShaderCompilerCLI")
    if (EXISTS ${shader_compiler_cli})
        add_executable(ShaderCompilerCLI IMPORTED GLOBAL)
        set_target_properties(ShaderCompilerCLI PROPERTIES
            IMPORTED_LOCATION
                "${shader_compiler_cli}"
        )
    endif()
    return()
endif()

add_executable(ShaderCompilerCLI
    ${project_root}/src/FlyCube/HLSLCompiler/Compiler.cpp
    ${project_root}/src/FlyCube/HLSLCompiler/DXCLoader.cpp
    ${project_root}/src/FlyCube/Utilities/SystemUtils.cpp
    main.cpp
)

target_link_libraries(ShaderCompilerCLI
    dxc
    gli
    glm
    nowide
)

target_include_directories(ShaderCompilerCLI
    PUBLIC
        "${project_root}/src/FlyCube"
)

set_target_properties(ShaderCompilerCLI PROPERTIES FOLDER "Tools")
