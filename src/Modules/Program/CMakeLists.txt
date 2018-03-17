set(target Program)

set(headers
    BufferLayout.h
    DX11ProgramApi.h
    DX12ProgramApi.h
    Program.h
    ProgramApi.h
    ShaderBase.h
    ShaderType.h
)

set(sources
    DX11ProgramApi.cpp
    DX12ProgramApi.cpp
)

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        "${project_root}/3rdparty/d3dx12"
)

target_link_libraries(${target}
    d3d11
    d3dcompiler
    Utilities
)

set_target_properties(${target} PROPERTIES FOLDER "library")
