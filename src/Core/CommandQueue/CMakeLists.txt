list(APPEND headers
    CommandQueue.h
    CommandQueueBase.h
)

list(APPEND sources
    CommandQueueBase.cpp
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKCommandQueue.h
    )
    list(APPEND sources
        VKCommandQueue.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXCommandQueue.h
    )
    list(APPEND sources
        DXCommandQueue.cpp
    )
endif()

add_library(CommandQueue ${headers} ${sources})

target_include_directories(CommandQueue
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(CommandQueue
    ApiType
    Swapchain
    Shader
    CommandList
    Program
    Framebuffer
    Pipeline
    CPUDescriptorPool
    GPUDescriptorPool
    Fence
    Utilities
    spirv-cross-core
    spirv-cross-hlsl
)

if (VULKAN_SUPPORT)
    target_link_libraries(CommandQueue
        ${Vulkan_LIBRARIES}
    )
    target_include_directories(CommandQueue
        PUBLIC
            ${Vulkan_INCLUDE_DIR}
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(CommandQueue
        d3d11
        d3d12
        dxgi
        dxguid
    )
    target_include_directories(CommandQueue
        PUBLIC
            "${project_root}/3rdparty/unpacked/d3dx12"
    )
endif()

set_target_properties(CommandQueue PROPERTIES FOLDER "Core")
