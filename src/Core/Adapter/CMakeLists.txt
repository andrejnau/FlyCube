list(APPEND headers
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
)

if (VULKAN_SUPPORT)
    target_link_libraries(Adapter
        vulkan
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(Adapter
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(Adapter PROPERTIES FOLDER "Core")
