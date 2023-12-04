if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXCPUDescriptorHandle.h
        DXCPUDescriptorPool.h
        DXCPUDescriptorPoolTyped.h
    )
    list(APPEND sources
        DXCPUDescriptorHandle.cpp
        DXCPUDescriptorPool.cpp
        DXCPUDescriptorPoolTyped.cpp
    )
else()
    return()
endif()

add_library(CPUDescriptorPool ${headers} ${sources})

target_include_directories(CPUDescriptorPool
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(CPUDescriptorPool
    ApiType
    Swapchain
    CommandList
    Fence
    Utilities
)

target_link_libraries(CPUDescriptorPool
    d3d12
    dxgi
    dxguid
)

set_target_properties(CPUDescriptorPool PROPERTIES FOLDER "FlyCube")
