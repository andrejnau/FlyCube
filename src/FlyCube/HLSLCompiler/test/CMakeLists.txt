add_executable(HLSLCompilerTest main.cpp)
if (WIN32)
    set_target_properties(HLSLCompilerTest PROPERTIES
        LINK_FLAGS "/ENTRY:wmainCRTStartup"
    )
endif()
target_link_libraries(HLSLCompilerTest PRIVATE FlyCube Catch2WithMain FlyCubeAssets)
set_target_properties(HLSLCompilerTest PROPERTIES FOLDER "Tests")

add_test(NAME HLSLCompilerTest COMMAND HLSLCompilerTest)
