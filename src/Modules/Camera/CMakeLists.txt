set(target Camera)

set(headers
    Camera.h
)

set(sources
    Camera.cpp
)

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

set_target_properties(${target} PROPERTIES FOLDER "library")
