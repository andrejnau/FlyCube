add_executable(ShaderReflectionTest main.cpp)
target_link_options(ShaderReflectionTest
    PRIVATE
        $<$<BOOL:${WIN32}>:/ENTRY:wmainCRTStartup>
)
target_link_libraries(ShaderReflectionTest PRIVATE FlyCube Catch2WithMain FlyCubeAssets)
set_target_properties(ShaderReflectionTest PROPERTIES FOLDER "Tests")

add_test(NAME ShaderReflectionTest COMMAND ShaderReflectionTest)
