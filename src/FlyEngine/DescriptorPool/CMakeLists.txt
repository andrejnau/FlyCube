if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXDescriptorHandle.h
        DXDescriptorPool.h
        DXDescriptorPoolTyped.h
    )
    list(APPEND sources
        DXDescriptorHandle.cpp
        DXDescriptorPool.cpp
        DXDescriptorPoolTyped.cpp
    )
endif()

add_library(DescriptorPool ${headers} ${sources})

target_include_directories(DescriptorPool
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(DescriptorPool
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
    target_link_libraries(DescriptorPool
        ${Vulkan_LIBRARIES}
    )
    target_include_directories(DescriptorPool
        PUBLIC
            ${Vulkan_INCLUDE_DIR}
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(DescriptorPool
        d3d11
        d3d12
        dxgi
        dxguid
    )
    target_include_directories(DescriptorPool
        PUBLIC
            "${project_root}/3rdparty/d3dx12"
    )
endif()

set_target_properties(DescriptorPool PROPERTIES FOLDER "FlyEngine")
