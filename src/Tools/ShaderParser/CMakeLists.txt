set(target ShaderParser)

set(source_path "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(${target}
    ${source_path}/templates/program.in
    ${source_path}/main.cpp
)

target_link_libraries(${target}
    Utilities
    HLSLCompiler
    mustache
)

set_target_properties(${target} PROPERTIES FOLDER "Tools")
