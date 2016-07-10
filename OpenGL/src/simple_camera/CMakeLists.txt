set (target simple_camera)

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}/include/${target}")
set(source_path "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(headers
    ${include_path}/camera.h
)

set(sources
    ${source_path}/camera.cpp
)

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(${target} PROPERTIES FOLDER "library")