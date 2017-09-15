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
    empty.cpp
)

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(${target}
    Utilities
    D3DUtils
    d3dcompiler
    d3d11
    gl_load
)

set_target_properties(${target} PROPERTIES FOLDER "library")
