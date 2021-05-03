set(target ShaderParser)

set(source_path "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(${target}
    ${source_path}/templates/shader.in
    ${source_path}/main.cpp
    ${source_path}/ShaderParser.h
    ${source_path}/ShaderParser.cpp
)

target_link_libraries(${target}
    HLSLCompiler
    ShaderReflection
    mustache
)

set_target_properties(${target} PROPERTIES FOLDER "Tools")
