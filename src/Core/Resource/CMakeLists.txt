list(APPEND headers
    Resource.h
    ResourceBase.h
    ResourceStateTracker.h
)

list(APPEND sources
    ResourceBase.cpp
    ResourceStateTracker.cpp
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
        DXResource.h
    )
    list(APPEND sources
        DXResource.cpp
    )
endif()

add_library(Resource ${headers} ${sources})

target_include_directories(Resource
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(Resource
    Device
    View
    Utilities
)

if (VULKAN_SUPPORT)
    target_link_libraries(Resource
        ${Vulkan_LIBRARIES}
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

set_target_properties(Resource PROPERTIES FOLDER "Core")
