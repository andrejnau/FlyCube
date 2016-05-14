set (PROJECT mlogger)
project (${PROJECT})

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(source_path "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(headers
    ${include_path}/mlogger.h
)

set(sources
    ${source_path}/mlogger.cpp
)

add_library(${PROJECT} ${headers} ${sources})

target_include_directories(${PROJECT}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(${PROJECT} PROPERTIES FOLDER "common")