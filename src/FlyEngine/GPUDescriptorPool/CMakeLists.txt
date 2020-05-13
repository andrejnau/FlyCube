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
    Semaphore
    Utilities
    spirv-cross-core
    spirv-cross-hlsl
    glad
)

if (VULKAN_SUPPORT)
    target_link_libraries(GPUDescriptorPool
        ${Vulkan_LIBRARIES}
    )
    target_include_directories(GPUDescriptorPool
        PUBLIC
            ${Vulkan_INCLUDE_DIR}
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(GPUDescriptorPool
        d3d11
        d3d12
        dxgi
        dxguid
    )
    target_include_directories(GPUDescriptorPool
        PUBLIC
            "${project_root}/3rdparty/d3dx12"
    )
endif()

set_target_properties(GPUDescriptorPool PROPERTIES FOLDER "FlyEngine")
