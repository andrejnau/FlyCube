list(APPEND headers
    BindingSetLayout.h
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKBindingSetLayout.h
    )
    list(APPEND sources
        VKBindingSetLayout.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXBindingSetLayout.h
    )
    list(APPEND sources
        DXBindingSetLayout.cpp
    )
endif()

if (METAL_SUPPORT)
    list(APPEND headers
        MTBindingSetLayout.h
    )
    list(APPEND sources
        MTBindingSetLayout.mm
    )
endif()

if (SOFTWARE_SUPPORT)
    list(APPEND headers
        SWBindingSetLayout.h
    )
    list(APPEND sources
        SWBindingSetLayout.cpp
    )
endif()

add_library(BindingSetLayout ${headers} ${sources})

target_include_directories(BindingSetLayout
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(BindingSetLayout
    ApiType
    Device
    Instance
    Utilities
)

if (VULKAN_SUPPORT)
    target_link_libraries(BindingSetLayout
        vulkan
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(BindingSetLayout
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(BindingSetLayout PROPERTIES FOLDER "Core")
