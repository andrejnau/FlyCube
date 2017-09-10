set(target AppBox)

set(headers
    AppBox.h
)

set(sources
    AppBox.cpp
)

add_library(${target} ${headers} ${sources})

target_link_libraries(${target}
    ISample
    glfw
)

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

set_target_properties(${target} PROPERTIES FOLDER "library")