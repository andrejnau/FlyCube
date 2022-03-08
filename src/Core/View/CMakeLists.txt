list(APPEND headers
    View.h
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKView.h
    )
    list(APPEND sources
        VKView.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXView.h
    )
    list(APPEND sources
        DXView.cpp
    )
endif()

if (METAL_SUPPORT)
    list(APPEND headers
        MTView.h
    )
    list(APPEND sources
        MTView.mm
    )
endif()

add_library(View ${headers} ${sources})

target_include_directories(View
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(View
    Resource
    Instance
    Utilities
)

if (VULKAN_SUPPORT)
    target_link_libraries(View
        vulkan
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(View
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(View PROPERTIES FOLDER "Core")
