set(headers
    Adapter.h
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKAdapter.h
    )
    list(APPEND sources
        VKAdapter.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXAdapter.h
    )
    list(APPEND sources
        DXAdapter.cpp
    )
endif()

add_library(Adapter ${headers} ${sources})

target_include_directories(Adapter
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(Adapter
    ApiType
    Device
    Utilities
    spirv-cross-core
    spirv-cross-hlsl
    glad
)

if (VULKAN_SUPPORT)
    target_link_libraries(Adapter
        ${Vulkan_LIBRARIES}
        VulkanExtLoader
    )
    target_include_directories(Adapter
        PUBLIC
            ${Vulkan_INCLUDE_DIR}
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(Adapter
        d3d11
        d3d12
        dxgi
        dxguid
    )
    target_include_directories(Adapter
        PUBLIC
            "${project_root}/3rdparty/d3dx12"
    )
endif()

set_target_properties(Adapter PROPERTIES FOLDER "FlyEngine")
