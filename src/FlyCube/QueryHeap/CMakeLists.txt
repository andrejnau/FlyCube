list(APPEND headers
    QueryHeap.h
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKQueryHeap.h
    )
    list(APPEND sources
        VKQueryHeap.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXRayTracingQueryHeap.h
    )
    list(APPEND sources
        DXRayTracingQueryHeap.cpp
    )
endif()

if (NOT sources)
    return()
endif()

add_library(QueryHeap ${headers} ${sources})

target_include_directories(QueryHeap
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(QueryHeap
    ApiType
    Device
    Utilities
)

if (VULKAN_SUPPORT)
    target_link_libraries(QueryHeap
        vulkan
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(QueryHeap
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(QueryHeap PROPERTIES FOLDER "FlyCube")
