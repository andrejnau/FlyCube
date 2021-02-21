if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXCPUDescriptorHandle.h
        DXCPUDescriptorPool.h
        DXCPUDescriptorPoolTyped.h
    )
    list(APPEND sources
        DXCPUDescriptorHandle.cpp
        DXCPUDescriptorPool.cpp
        DXCPUDescriptorPoolTyped.cpp
    )
endif()

add_library(CPUDescriptorPool ${headers} ${sources})

target_include_directories(CPUDescriptorPool
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(CPUDescriptorPool
    ApiType
    Swapchain
    CommandList
    Fence
    Utilities
    spirv-cross-core
    spirv-cross-hlsl
)

if (VULKAN_SUPPORT)
    target_link_libraries(CPUDescriptorPool
        ${Vulkan_LIBRARIES}
    )
    target_include_directories(CPUDescriptorPool
        PUBLIC
            ${Vulkan_INCLUDE_DIR}
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(CPUDescriptorPool
        d3d11
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(CPUDescriptorPool PROPERTIES FOLDER "Core")
