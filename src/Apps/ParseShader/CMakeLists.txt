set(target ParseShader)

set(source_path "${CMAKE_CURRENT_SOURCE_DIR}")

set(sources
    ${source_path}/main.cpp
)

add_executable(${target} 
    ${source_path}/templates/program.in
    ${sources}
)

target_link_libraries(${target}
    Utilities
	Shader
    d3dcompiler
)
