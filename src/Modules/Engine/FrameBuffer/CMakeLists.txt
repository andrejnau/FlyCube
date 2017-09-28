set(target FrameBuffer)

set(headers
    FrameBuffer.h
)

set(sources
    FrameBuffer.cpp
)

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

set_target_properties(${target} PROPERTIES FOLDER "library/Engine")
