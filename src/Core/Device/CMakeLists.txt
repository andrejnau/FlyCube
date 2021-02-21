list(APPEND headers
    Device.h
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKDevice.h
    )
    list(APPEND sources
        VKDevice.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXDevice.h
    )
    list(APPEND sources
        DXDevice.cpp
    )
endif()

add_library(Device ${headers} ${sources})

target_include_directories(Device
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(Device
    ApiType
    Swapchain
    Shader
    CommandList
    CommandQueue
    Program
    Framebuffer
    Pipeline
    CPUDescriptorPool
    GPUDescriptorPool
    Fence
    Memory
    Utilities
    spirv-cross-core
    spirv-cross-hlsl
)

if (VULKAN_SUPPORT)
    target_link_libraries(Device
        ${Vulkan_LIBRARIES}
    )
    target_include_directories(Device
        PUBLIC
            ${Vulkan_INCLUDE_DIR}
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(Device
        d3d11
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(Device PROPERTIES FOLDER "Core")
