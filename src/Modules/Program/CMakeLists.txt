set(target Program)

set(headers
    ShaderType.h
    Program.h
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
