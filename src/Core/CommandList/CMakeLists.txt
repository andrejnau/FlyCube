list(APPEND headers
    CommandList.h
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKCommandList.h
    )
    list(APPEND sources
        VKCommandList.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXCommandList.h
    )
    list(APPEND sources
        DXCommandList.cpp
    )
endif()

if (VULKAN_SUPPORT)
    list(APPEND headers
        MTCommandList.h
    )
    list(APPEND sources
        MTCommandList.mm
    )
endif()

add_library(CommandList ${headers} ${sources})

target_include_directories(CommandList
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(CommandList
    ApiType
    Device
    Utilities
    nowide
)

if (VULKAN_SUPPORT)
    target_link_libraries(CommandList
        vulkan
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(CommandList
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(CommandList PROPERTIES FOLDER "Core")
