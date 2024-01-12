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

if (METAL_SUPPORT)
    list(APPEND headers
        MTRenderPass.h
    )
    list(APPEND sources
        MTRenderPass.mm
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
)

if (VULKAN_SUPPORT)
    target_link_libraries(RenderPass
        vulkan
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(RenderPass
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(RenderPass PROPERTIES FOLDER "FlyCube")
