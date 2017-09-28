set(target SwapChain)

set(headers
    SwapChain.h
)

set(sources
    SwapChain.cpp
)

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

set_target_properties(${target} PROPERTIES FOLDER "library/Engine")
