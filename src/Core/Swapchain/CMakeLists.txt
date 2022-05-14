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

if (METAL_SUPPORT)
    list(APPEND headers
        MTSwapchain.h
    )
    list(APPEND sources
        MTSwapchain.mm
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

if (APPLE)
    if (VULKAN_SUPPORT)
        set_property(SOURCE VKSwapchain.cpp PROPERTY COMPILE_FLAGS "-xobjective-c++")
    endif()
endif()

if (UNIX)
    target_link_libraries(Swapchain
        X11-xcb
    )
endif()

set_target_properties(Swapchain PROPERTIES FOLDER "Core")
