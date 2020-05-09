set(target Context)

set(headers
    BaseTypes.h
    Context.h
    ContextFly.h
    ContextSelector.h
    GLContext.h
)

set(sources
    Context.cpp
    ContextFly.cpp
    ContextSelector.cpp
    GLContext.cpp
)

if (VULKAN_SUPPORT)
    set(headers 
        ${headers}
        VKContext.h
        VKDescriptorPool.h
    )
    set(sources 
        ${sources}
        VKContext.cpp
        VKDescriptorPool.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    set(headers 
        ${headers}
        DX11Context.h
        DX12Context.h
        DXGIUtility.h
        DescriptorPool.h
    )
    set(sources 
        ${sources}
        DescriptorPool.cpp
        DX11Context.cpp
        DX12Context.cpp
        DXGIUtility.cpp
    )
endif()

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        "${project_root}/3rdparty/d3dx12"
        ${Vulkan_INCLUDE_DIR}
)

target_link_libraries(${target}
    ApiType
    Instance
    Utilities
    spirv-cross-core
    spirv-cross-hlsl
    glad
    Resource
    Program
    View
)

if (VULKAN_SUPPORT)
    target_link_libraries(${target}
        ${Vulkan_LIBRARIES}
        VulkanExtLoader
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(${target}
        d3d11
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(${target} PROPERTIES FOLDER "library")
