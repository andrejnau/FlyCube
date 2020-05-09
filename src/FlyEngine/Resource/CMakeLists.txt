list(APPEND headers
    GLResource.h
    Resource.h
)

list(APPEND sources
    Resource.cpp
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKResource.h
    )
    list(APPEND sources
        VKResource.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DX11Resource.h
        DX12Resource.h
    )
    list(APPEND sources
        DX12Resource.cpp
    )
endif()

add_library(Resource ${headers} ${sources})

if (VULKAN_SUPPORT)
    target_link_libraries(Resource
        ${Vulkan_LIBRARIES}
        VulkanExtLoader
    )
    target_include_directories(Resource
        PUBLIC
            ${Vulkan_INCLUDE_DIR}
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(Resource
        d3d11
        d3d12
        dxgi
        dxguid
    )
    target_include_directories(Resource
        PUBLIC
            "${project_root}/3rdparty/d3dx12"
    )
endif()

set_target_properties(Resource PROPERTIES FOLDER "FlyEngine")
