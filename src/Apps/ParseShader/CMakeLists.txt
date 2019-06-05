set(target ParseShader)

set(source_path "${CMAKE_CURRENT_SOURCE_DIR}")

if (WIN32)
    set(sources
        ${source_path}/main.cpp
    )
else()
    set(sources
        ${source_path}/main_linux.cpp
    )
endif()

add_executable(${target} 
    ${source_path}/templates/program.in
    ${sources}
)

target_link_libraries(${target}
    Utilities
    Shader
)
