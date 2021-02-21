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

add_library(CommandList ${headers} ${sources})

target_include_directories(CommandList
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(CommandList
    ApiType
    Device
    Utilities
    spirv-cross-core
    spirv-cross-hlsl
)

if (VULKAN_SUPPORT)
    target_link_libraries(CommandList
        ${Vulkan_LIBRARIES}
    )
    target_include_directories(CommandList
        PUBLIC
            ${Vulkan_INCLUDE_DIR}
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(CommandList
        d3d11
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(CommandList PROPERTIES FOLDER "Core")
