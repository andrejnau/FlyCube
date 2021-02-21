list(APPEND headers
    Instance.h
    BaseTypes.h
)

list(APPEND sources
    Instance.cpp
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKInstance.h
    )
    list(APPEND sources
        VKInstance.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXInstance.h
    )
    list(APPEND sources
        DXInstance.cpp
    )
endif()

add_library(Instance ${headers} ${sources})

target_include_directories(Instance
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(Instance
    Adapter
    ApiType
    Utilities
    spirv-cross-core
    spirv-cross-hlsl
)

if (VULKAN_SUPPORT)
    target_link_libraries(Instance
        ${Vulkan_LIBRARIES}
    )
    target_include_directories(Instance
        PUBLIC
            ${Vulkan_INCLUDE_DIR}
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(Instance
        d3d11
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(Instance PROPERTIES FOLDER "Core")
