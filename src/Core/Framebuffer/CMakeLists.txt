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
    spirv-cross-core
    spirv-cross-hlsl
    glad
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
        d3d11
        d3d12
        dxgi
        dxguid
    )
    target_include_directories(Framebuffer
        PUBLIC
            "${project_root}/3rdparty/d3dx12"
    )
endif()

set_target_properties(Framebuffer PROPERTIES FOLDER "Core")
