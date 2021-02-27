if (VULKAN_SUPPORT)
    list(APPEND headers
        SpirvCompiler.h
    )
    list(APPEND sources
        SpirvCompiler.cpp
    )
endif()

list(APPEND headers
    DXCompiler.h
    DXReflector.h
    DXCLoader.h
)
list(APPEND sources
    DXCompiler.cpp
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
)

set_target_properties(HLSLCompiler PROPERTIES FOLDER "Core")
