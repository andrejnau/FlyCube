set(target DX11Context)

set(headers
    DX11Context.h
)

set(sources
    DX11Context.cpp
)

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(${target}
    Context
    Utilities
)

set_target_properties(${target} PROPERTIES FOLDER "library")
