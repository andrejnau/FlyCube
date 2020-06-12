list(APPEND headers
    Semaphore.h
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKSemaphore.h
    )
    list(APPEND sources
        VKSemaphore.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXSemaphore.h
    )
    list(APPEND sources
        DXSemaphore.cpp
    )
endif()

add_library(Semaphore ${headers} ${sources})

target_include_directories(Semaphore
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(Semaphore
    ApiType
    Device
    Utilities
    spirv-cross-core
    spirv-cross-hlsl
)

if (VULKAN_SUPPORT)
    target_link_libraries(Semaphore
        ${Vulkan_LIBRARIES}
    )
    target_include_directories(Semaphore
        PUBLIC
            ${Vulkan_INCLUDE_DIR}
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(Semaphore
        d3d11
        d3d12
        dxgi
        dxguid
    )
    target_include_directories(Semaphore
        PUBLIC
            "${project_root}/3rdparty/unpacked/d3dx12"
    )
endif()

set_target_properties(Semaphore PROPERTIES FOLDER "Core")
