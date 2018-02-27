set(target Program)

set(headers
    DX11ProgramApi.h
    DX12ProgramApi.h
    Program.h
    ProgramApi.h
    ShaderBase.h
    ShaderType.h
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
    d3dcompiler
    d3d11
    glLoadGen
)

set_target_properties(${target} PROPERTIES FOLDER "library")
