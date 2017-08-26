set (PROJECT gl_load)
project (${PROJECT})

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}")
set(source_path "${CMAKE_CURRENT_SOURCE_DIR}")

set(headers
    ${include_path}/gl_core_4_5.h
)

set(sources
    ${source_path}/gl_core_4_5.c
)

add_library(${PROJECT} ${headers} ${sources})

target_include_directories(${PROJECT}
    PUBLIC
        ${include_path}
)

set_target_properties(${PROJECT} PROPERTIES FOLDER "library")