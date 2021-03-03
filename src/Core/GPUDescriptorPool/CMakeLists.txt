if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXGPUDescriptorPool.h
        DXGPUDescriptorPoolRange.h
        DXGPUDescriptorPoolTyped.h
    )
    list(APPEND sources
        DXGPUDescriptorPool.cpp
        DXGPUDescriptorPoolRange.cpp
        DXGPUDescriptorPoolTyped.cpp
    )
endif()

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKGPUDescriptorPool.h
        VKGPUBindlessDescriptorPoolTyped.cpp
        VKGPUDescriptorPoolRange.h
    )
    list(APPEND sources
        VKGPUDescriptorPool.cpp
        VKGPUBindlessDescriptorPoolTyped.cpp
        VKGPUDescriptorPoolRange.cpp
    )
endif()

add_library(GPUDescriptorPool ${headers} ${sources})

target_include_directories(GPUDescriptorPool
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(GPUDescriptorPool
    ApiType
    Swapchain
    CommandList
    Fence
    Utilities
)

if (VULKAN_SUPPORT)
    target_link_libraries(GPUDescriptorPool
        vulkan
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(GPUDescriptorPool
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(GPUDescriptorPool PROPERTIES FOLDER "Core")
