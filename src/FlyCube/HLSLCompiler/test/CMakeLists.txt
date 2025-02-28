add_executable(HLSLCompilerTest main.cpp)
if (APPLE)
    set_property(SOURCE main.cpp PROPERTY COMPILE_FLAGS "-xobjective-c++")
endif()
target_link_options(HLSLCompilerTest
    PRIVATE
        $<$<BOOL:${WIN32}>:/ENTRY:wmainCRTStartup>
)
target_link_libraries(HLSLCompilerTest PRIVATE Catch2WithMain FlyCube FlyCubeAssets)
set_target_properties(HLSLCompilerTest PROPERTIES FOLDER "Tests")

add_test(NAME HLSLCompilerTest COMMAND HLSLCompilerTest)
