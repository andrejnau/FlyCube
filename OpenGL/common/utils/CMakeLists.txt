set (PROJECT utils)
project (${PROJECT})

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(headers
    ${include_path}/platform.h
    ${include_path}/scenebase.h
    ${include_path}/singleton.h
    ${include_path}/state.h
    ${include_path}/utilities.h
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