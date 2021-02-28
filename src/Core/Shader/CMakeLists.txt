list(APPEND headers
    Shader.h
    ShaderBase.h
)

list(APPEND sources
    ShaderBase.cpp
)

add_library(Shader ${headers} ${sources})

target_include_directories(Shader
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(Shader
    Utilities
    HLSLCompiler
    ShaderReflection
)

set_target_properties(Shader PROPERTIES FOLDER "Core")
