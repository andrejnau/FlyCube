set(target DX11Program)

set(headers
    DX11Program.h
    DX11ShaderBuffer.h
    ShaderType.h
)

set(sources
    DX11Program.cpp
    DX11ShaderBuffer.cpp
)

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(${target}
    Utilities
    d3dcompiler
    Program
    d3d11
    DX11Geometry
)

set_target_properties(${target} PROPERTIES FOLDER "library")
