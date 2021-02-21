list(APPEND headers
    Memory.h
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKMemory.h
    )
    list(APPEND sources
        VKMemory.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXMemory.h
    )
    list(APPEND sources
        DXMemory.cpp
    )
endif()

add_library(Memory ${headers} ${sources})

target_include_directories(Memory
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(Memory
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
    Utilities
    spirv-cross-core
    spirv-cross-hlsl
)

if (VULKAN_SUPPORT)
    target_link_libraries(Memory
        ${Vulkan_LIBRARIES}
    )
    target_include_directories(Memory
        PUBLIC
            ${Vulkan_INCLUDE_DIR}
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(Memory
        d3d11
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(Memory PROPERTIES FOLDER "Core")
