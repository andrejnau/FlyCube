if (CMAKE_CROSSCOMPILING)
    include(${PROJECT_BINARY_DIR}/hostPlatform/src/Tools/ShaderCompilerCLI/ShaderCompilerCLI.cmake)
    get_target_property(shader_compiler_cli ShaderCompilerCLI::ShaderCompilerCLI IMPORTED_LOCATION_RELEASE)
    if (EXISTS ${shader_compiler_cli})
        add_executable(ShaderCompilerCLI IMPORTED GLOBAL)
        set_target_properties(ShaderCompilerCLI PROPERTIES
            IMPORTED_LOCATION
                "${shader_compiler_cli}"
        )
    else()
        message(FATAL_ERROR "ShaderCompilerCLI (${shader_compiler_cli}) is not exist")
    endif()
    return()
endif()

add_executable(ShaderCompilerCLI
    ${project_root}/src/FlyCube/HLSLCompiler/Compiler.cpp
    ${project_root}/src/FlyCube/HLSLCompiler/DXCLoader.cpp
    ${project_root}/src/FlyCube/Utilities/Logging.cpp
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

export(TARGETS ShaderCompilerCLI NAMESPACE ShaderCompilerCLI:: FILE ShaderCompilerCLI.cmake)
