list(APPEND headers
    Shader.h
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        SpirvShader.h
    )
    list(APPEND sources
        SpirvShader.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXShader.h
    )
    list(APPEND sources
        DXShader.cpp
    )
endif()

add_library(Shader ${headers} ${sources})

target_include_directories(Shader
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(Shader
    Utilities
    HLSLCompiler
)

set_target_properties(Shader PROPERTIES FOLDER "Core")
