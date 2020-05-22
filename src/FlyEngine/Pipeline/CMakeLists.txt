list(APPEND headers
    Pipeline.h
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKGraphicsPipeline.h
    )
    list(APPEND sources
        VKGraphicsPipeline.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXComputePipeline.h
        DXGraphicsPipeline.h
    )
    list(APPEND sources
        DXComputePipeline.cpp
        DXGraphicsPipeline.cpp
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
    spirv-cross-core
    spirv-cross-hlsl
    glad
)

if (VULKAN_SUPPORT)
    target_link_libraries(Pipeline
        ${Vulkan_LIBRARIES}
    )
    target_include_directories(Pipeline
        PUBLIC
            ${Vulkan_INCLUDE_DIR}
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(Pipeline
        d3d11
        d3d12
        dxgi
        dxguid
    )
    target_include_directories(Pipeline
        PUBLIC
            "${project_root}/3rdparty/d3dx12"
    )
endif()

set_target_properties(Pipeline PROPERTIES FOLDER "FlyEngine")
