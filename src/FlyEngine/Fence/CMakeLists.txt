list(APPEND headers
    Fence.h
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKTimelineSemaphore.h
    )
    list(APPEND sources
        VKTimelineSemaphore.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXFence.h
    )
    list(APPEND sources
        DXFence.cpp
    )
endif()

add_library(Fence ${headers} ${sources})

target_include_directories(Fence
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(Fence
    ApiType
    Device
    Utilities
    spirv-cross-core
    spirv-cross-hlsl
    glad
)

if (VULKAN_SUPPORT)
    target_link_libraries(Fence
        ${Vulkan_LIBRARIES}
    )
    target_include_directories(Fence
        PUBLIC
            ${Vulkan_INCLUDE_DIR}
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(Fence
        d3d11
        d3d12
        dxgi
        dxguid
    )
    target_include_directories(Fence
        PUBLIC
            "${project_root}/3rdparty/d3dx12"
    )
endif()

set_target_properties(Fence PROPERTIES FOLDER "FlyEngine")
