set(target Texture)

set(headers
    Texture.h
)

set(sources
    Texture.cpp
)

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

set_target_properties(${target} PROPERTIES FOLDER "library/Engine")
