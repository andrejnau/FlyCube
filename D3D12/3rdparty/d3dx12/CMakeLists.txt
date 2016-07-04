set (PROJECT d3dx12)
project (${PROJECT})

set(headers
    SimpleMath.h
)

set(sources
    SimpleMath.cpp
)

add_library(${PROJECT} ${headers} ${sources})

target_include_directories(${PROJECT}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(${PROJECT} PROPERTIES FOLDER "3rdparty")