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

if (METAL_SUPPORT)
    list(APPEND headers
        MTMemory.h
    )
    list(APPEND sources
        MTMemory.mm
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
    Fence
    Utilities
)

if (VULKAN_SUPPORT)
    target_link_libraries(Memory
        vulkan
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(Memory
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(Memory PROPERTIES FOLDER "Core")
