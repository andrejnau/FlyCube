list(APPEND headers
    BindingSet.h
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKBindingSet.h
    )
    list(APPEND sources
        VKBindingSet.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXBindingSet.h
    )
    list(APPEND sources
        DXBindingSet.cpp
    )
endif()

add_library(BindingSet ${headers} ${sources})

target_include_directories(BindingSet
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(BindingSet
    ApiType
    Device
    Instance
    Utilities
    spirv-cross-core
    spirv-cross-hlsl
    glad
)

if (VULKAN_SUPPORT)
    target_link_libraries(BindingSet
        ${Vulkan_LIBRARIES}
    )
    target_include_directories(BindingSet
        PUBLIC
            ${Vulkan_INCLUDE_DIR}
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(BindingSet
        d3d11
        d3d12
        dxgi
        dxguid
    )
    target_include_directories(BindingSet
        PUBLIC
            "${project_root}/3rdparty/d3dx12"
    )
endif()

set_target_properties(BindingSet PROPERTIES FOLDER "Core")
