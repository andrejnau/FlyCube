list(APPEND headers
    Framebuffer.h
    FramebufferBase.h
)

list(APPEND sources
    FramebufferBase.cpp
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKFramebuffer.h
    )
    list(APPEND sources
        VKFramebuffer.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXFramebuffer.h
    )
endif()

add_library(Framebuffer ${headers} ${sources})

target_include_directories(Framebuffer
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(Framebuffer
    ApiType
    Device
    Utilities
)

if (VULKAN_SUPPORT)
    target_link_libraries(Framebuffer
        ${Vulkan_LIBRARIES}
    )
    target_include_directories(Framebuffer
        PUBLIC
            ${Vulkan_INCLUDE_DIR}
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(Framebuffer
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(Framebuffer PROPERTIES FOLDER "Core")
