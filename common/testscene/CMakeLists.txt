set (PROJECT testscene)
project (${PROJECT})

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(headers
    ${include_path}/geometry.h
    ${include_path}/shaders.h
    ${include_path}/testscene.h
)

add_library(${PROJECT} INTERFACE)

target_include_directories(${PROJECT}
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources(${PROJECT}
    INTERFACE
        ${headers}
)