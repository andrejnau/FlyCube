list(APPEND headers
    Compiler.h
    DXCLoader.h
)
list(APPEND sources
    Compiler.cpp
    DXCLoader.cpp
    DXCWinAdapter.cpp
)

if (METAL_SUPPORT)
    list(APPEND headers
        MSLConverter.h
    )
    list(APPEND sources
        MSLConverter.cpp
    )
endif()

add_library(HLSLCompiler ${headers} ${sources})

target_include_directories(HLSLCompiler
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(HLSLCompiler
    Utilities
    nowide
)

if (VULKAN_SUPPORT)
    target_link_libraries(HLSLCompiler
        spirv-cross-core
        spirv-cross-hlsl
    )
endif()

if (METAL_SUPPORT)
    target_link_libraries(HLSLCompiler
        spirv-cross-msl
    )
endif()

if (UNIX)
    target_link_libraries(HLSLCompiler
        dl
    )
endif()

target_link_libraries(HLSLCompiler
    dxc
)

set_target_properties(HLSLCompiler PROPERTIES FOLDER "Core")

add_subdirectory(test)
