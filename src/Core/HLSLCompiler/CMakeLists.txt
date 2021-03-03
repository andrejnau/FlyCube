list(APPEND headers
    Compiler.h
    DXReflector.h
    DXCLoader.h
)
list(APPEND sources
    Compiler.cpp
    DXReflector.cpp
    DXCLoader.cpp
)

add_library(HLSLCompiler ${headers} ${sources})

target_include_directories(HLSLCompiler
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(HLSLCompiler
    Utilities
)

if (VULKAN_SUPPORT)
    target_link_libraries(HLSLCompiler
        spirv-cross-core
        spirv-cross-hlsl
    )
endif()

target_link_libraries(HLSLCompiler
    d3dcompiler
    dxc
)

set_target_properties(HLSLCompiler PROPERTIES FOLDER "Core")
