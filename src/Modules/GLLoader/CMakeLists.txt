set(target GLLoader)

set(headers
    GL.h
    gl_core_4_5.h
)

set(sources
    gl_core_4_5.c
)

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

set_target_properties(${target} PROPERTIES FOLDER "library")
