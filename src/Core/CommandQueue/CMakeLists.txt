list(APPEND headers
    CommandQueue.h
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

if (METAL_SUPPORT)
    list(APPEND headers
        MTCommandQueue.h
    )
    list(APPEND sources
        MTCommandQueue.mm
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
    GPUDescriptorPool
    Fence
    Utilities
)

if (VULKAN_SUPPORT)
    target_link_libraries(CommandQueue
        vulkan
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(CommandQueue
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(CommandQueue PROPERTIES FOLDER "Core")
