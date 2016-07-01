set (PROJECT simple_camera)
project (${PROJECT})

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT}")
set(source_path "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(headers
    ${include_path}/camera.h
)

set(sources
    ${source_path}/camera.cpp
)

add_library(${PROJECT} ${headers} ${sources})

target_include_directories(${PROJECT}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(${PROJECT} PROPERTIES FOLDER "common")