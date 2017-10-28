set(target Program)

set(headers
    Program.h
    BufferProxy.h
    IShaderBuffer.h
)

set(sources
    Program.cpp
)

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

set_target_properties(${target} PROPERTIES FOLDER "library/Engine")
