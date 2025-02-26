add_executable(HLSLCompilerTest main.cpp)
target_link_options(HLSLCompilerTest
    PRIVATE
        $<$<BOOL:${WIN32}>:/ENTRY:wmainCRTStartup>
)
target_link_libraries(HLSLCompilerTest PRIVATE FlyCube Catch2WithMain FlyCubeAssets)
set_target_properties(HLSLCompilerTest PROPERTIES FOLDER "Tests")

add_test(NAME HLSLCompilerTest COMMAND HLSLCompilerTest)
