add_executable(HLSLCompilerTest main.cpp)
if (WIN32)
    set_target_properties(HLSLCompilerTest PROPERTIES
        LINK_FLAGS "/ENTRY:wmainCRTStartup"
    )
endif()
target_link_libraries(HLSLCompilerTest PRIVATE HLSLCompiler Catch2WithMain FlyCubeAssets)
set_target_properties(HLSLCompilerTest PROPERTIES FOLDER "Tests")
