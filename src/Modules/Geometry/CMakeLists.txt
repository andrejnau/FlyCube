set(target Geometry)

set(headers
    Geometry.h
)

set(sources
    Geometry.cpp
)

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(${target}
    Utilities
)

set_target_properties(${target} PROPERTIES FOLDER "library")
