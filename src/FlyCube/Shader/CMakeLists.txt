list(APPEND headers
    Shader.h
    ShaderBase.h
)

list(APPEND sources
    ShaderBase.cpp
)

if (METAL_SUPPORT)
    list(APPEND headers
        MTShader.h
    )
    list(APPEND sources
        MTShader.mm
    )
endif()

add_library(Shader ${headers} ${sources})

target_include_directories(Shader
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(Shader
    Device
    HLSLCompiler
    ShaderReflection
    Utilities
)

set_target_properties(Shader PROPERTIES FOLDER "FlyCube")
