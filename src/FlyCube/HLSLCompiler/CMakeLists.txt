list(APPEND headers
    Compiler.h
    DXCLoader.h
    MSLConverter.h
)
list(APPEND sources
    Compiler.cpp
    DXCLoader.cpp
    MSLConverter.cpp
)

add_library(HLSLCompiler ${headers} ${sources})

target_include_directories(HLSLCompiler
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(HLSLCompiler
    Utilities
    nowide
    spirv-cross-core
    spirv-cross-hlsl
    spirv-cross-msl
    dxc
)

if (UNIX)
    target_link_libraries(HLSLCompiler
        dl
    )
endif()

set_target_properties(HLSLCompiler PROPERTIES FOLDER "FlyCube")

if (BUILD_TESTING)
    add_subdirectory(test)
endif()
