list(APPEND headers
    RenderPass.h
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKRenderPass.h
    )
    list(APPEND sources
        VKRenderPass.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXRenderPass.h
    )
    list(APPEND sources
        DXRenderPass.cpp
    )
endif()

add_library(RenderPass ${headers} ${sources})

target_include_directories(RenderPass
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(RenderPass
    ApiType
    Device
    Utilities
    spirv-cross-core
    spirv-cross-hlsl
)

if (VULKAN_SUPPORT)
    target_link_libraries(RenderPass
        ${Vulkan_LIBRARIES}
    )
    target_include_directories(RenderPass
        PUBLIC
            ${Vulkan_INCLUDE_DIR}
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(RenderPass
        d3d11
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(RenderPass PROPERTIES FOLDER "Core")
