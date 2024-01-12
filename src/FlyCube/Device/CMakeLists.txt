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

if (METAL_SUPPORT)
    list(APPEND headers
        MTDevice.h
    )
    list(APPEND sources
        MTDevice.mm
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
    Fence
    Memory
    Utilities
)

if (VULKAN_SUPPORT)
    target_link_libraries(Device
        vulkan
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(Device
        CPUDescriptorPool
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(Device PROPERTIES FOLDER "FlyCube")
