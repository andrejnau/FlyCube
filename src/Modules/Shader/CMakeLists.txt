set(target Shader)

set(headers
    ShaderBase.h
    ShaderDesc.h
    SpirvCompiler.h
    GLSLConverter.h
)

set(sources
    SpirvCompiler.cpp
    GLSLConverter.cpp
)

if (DIRECTX_SUPPORT)
    set(headers 
        ${headers}
        DXCompiler.h
        DXReflector.h
        DXCLoader.h
    )
    set(sources 
        ${sources}
        DXCompiler.cpp
        DXReflector.cpp
        DXCLoader.cpp
    )
endif()

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        "${project_root}/3rdparty/d3dx12"
        ${Vulkan_INCLUDE_DIR}
)

target_link_libraries(${target}
    Utilities
    spirv-cross-core
    spirv-cross-hlsl
)

if (VULKAN_SUPPORT)
    target_link_libraries(${target}
        ${Vulkan_LIBRARIES}
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(${target}
        d3dcompiler
    )
endif()

set_target_properties(${target} PROPERTIES FOLDER "library")
