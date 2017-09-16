set(target Program)

set(headers
    DX11Program.h
    GLProgram.h
    IShaderBuffer.h
    DX11ShaderBuffer.h
    GLShaderBuffer.h
    ShaderType.h
    BufferProxy.h
    GLUniformProxy.h
)

set(sources
    DX11Program.cpp
    DX11ShaderBuffer.cpp
    GLProgram.cpp
    GLShaderBuffer.cpp
)

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(${target}
    Utilities
    d3dcompiler
    d3d11
    GLLoader
)

set_target_properties(${target} PROPERTIES FOLDER "library")
