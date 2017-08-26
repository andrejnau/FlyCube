set (target utils)

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(headers
    ${include_path}/platform.h
    ${include_path}/scenebase.h
    ${include_path}/singleton.h
    ${include_path}/state.h
    ${include_path}/utilities.h
    ${include_path}/mlogger.h
)

add_library(${target} INTERFACE)

target_include_directories(${target}
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources(${target}
    INTERFACE
        ${headers}
)