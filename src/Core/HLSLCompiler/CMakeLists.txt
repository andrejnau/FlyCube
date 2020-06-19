if (VULKAN_SUPPORT)
    list(APPEND headers
        SpirvCompiler.h
    )
    list(APPEND sources
        SpirvCompiler.cpp
    )
endif()

if (DIRECTX_SUPPORT)
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
endif()

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

if (VULKAN_SUPPORT AND USE_SHADERC)
    target_link_libraries(HLSLCompiler
        ${Vulkan_INCLUDE_DIR}/../Lib/shaderc_shared.lib
    )
    target_include_directories(HLSLCompiler
        PUBLIC
            ${Vulkan_INCLUDE_DIR}
    )
    target_compile_definitions(HLSLCompiler PRIVATE -DUSE_SHADERC)
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(HLSLCompiler
        d3dcompiler
    )
endif()

set_target_properties(HLSLCompiler PROPERTIES FOLDER "Core")
