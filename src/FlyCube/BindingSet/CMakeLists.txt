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

if (METAL_SUPPORT)
    list(APPEND headers
        MTBindingSet.h
    )
    list(APPEND sources
        MTBindingSet.mm
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
    BindingSetLayout
)

if (VULKAN_SUPPORT)
    target_link_libraries(BindingSet
        vulkan
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(BindingSet
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(BindingSet PROPERTIES FOLDER "FlyCube")
