list(APPEND headers
    Pipeline.h
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKPipeline.h
        VKComputePipeline.h
        VKGraphicsPipeline.h
        VKRayTracingPipeline.h
    )
    list(APPEND sources
        VKPipeline.cpp
        VKComputePipeline.cpp
        VKGraphicsPipeline.cpp
        VKRayTracingPipeline.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXPipeline.h
        DXComputePipeline.h
        DXGraphicsPipeline.h
        DXRayTracingPipeline.h
        DXStateBuilder.h
    )
    list(APPEND sources
        DXPipeline.cpp
        DXComputePipeline.cpp
        DXGraphicsPipeline.cpp
        DXRayTracingPipeline.cpp
    )
endif()

if (METAL_SUPPORT)
    list(APPEND headers
        MTPipeline.h
        MTComputePipeline.h
        MTGraphicsPipeline.h
    )
    list(APPEND sources
        MTComputePipeline.mm
        MTGraphicsPipeline.mm
    )
endif()

add_library(Pipeline ${headers} ${sources})

target_include_directories(Pipeline
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(Pipeline
    ApiType
    Device
    Utilities
    Program
    RenderPass
    Shader
)

if (VULKAN_SUPPORT)
    target_link_libraries(Pipeline
        vulkan
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(Pipeline
        d3d12
        dxgi
        dxguid
    )
endif()

if (METAL_SUPPORT)
    target_link_libraries(Pipeline
        MVKPixelFormats
    )
endif()

set_target_properties(Pipeline PROPERTIES FOLDER "FlyCube")
