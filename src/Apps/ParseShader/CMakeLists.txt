set(target ParseShader)

set(source_path "${CMAKE_CURRENT_SOURCE_DIR}")

set(sources
    ${source_path}/main.cpp
)

add_executable(${target} ${sources})

target_compile_definitions(${target} PRIVATE UNICODE _UNICODE NOMINMAX)

target_link_libraries(${target}
    Utilities
    d3dcompiler
)
