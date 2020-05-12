list(APPEND headers
    PipelineState.h
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKPipelineState.h
    )
    list(APPEND sources
        VKPipelineState.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXPipelineState.h
    )
    list(APPEND sources
        DXPipelineState.cpp
    )
endif()

add_library(PipelineState ${headers} ${sources})

target_include_directories(PipelineState
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(PipelineState
    ApiType
    Device
    Utilities
    PipelineProgram
    Shader
    spirv-cross-core
    spirv-cross-hlsl
    glad
)

if (VULKAN_SUPPORT)
    target_link_libraries(PipelineState
        ${Vulkan_LIBRARIES}
    )
    target_include_directories(PipelineState
        PUBLIC
            ${Vulkan_INCLUDE_DIR}
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(PipelineState
        d3d11
        d3d12
        dxgi
        dxguid
    )
    target_include_directories(PipelineState
        PUBLIC
            "${project_root}/3rdparty/d3dx12"
    )
endif()

set_target_properties(PipelineState PROPERTIES FOLDER "FlyEngine")
