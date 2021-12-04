list(APPEND headers
    Swapchain.h
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKSwapchain.h
    )
    list(APPEND sources
        VKSwapchain.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXSwapchain.h
    )
    list(APPEND sources
        DXSwapchain.cpp
    )
endif()

add_library(Swapchain ${headers} ${sources})

target_include_directories(Swapchain
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(Swapchain
    ApiType
    Device
    Resource
    Utilities
    glfw
)

if (VULKAN_SUPPORT)
    target_link_libraries(Swapchain
        vulkan
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(Swapchain
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(Swapchain PROPERTIES FOLDER "Core")
