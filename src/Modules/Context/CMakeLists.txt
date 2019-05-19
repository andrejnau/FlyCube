set(target Context)

set(headers
    BaseTypes.h
    Context.h
    ContextSelector.h
    DescriptorPool.h
    DX11Context.h
    DX12Context.h
    DXGIUtility.h
    GLContext.h
)

set(sources
    Context.cpp
    ContextSelector.cpp
    DescriptorPool.cpp
    DX11Context.cpp
    DX12Context.cpp
    DXGIUtility.cpp
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

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        "${project_root}/3rdparty/d3dx12"
        ${Vulkan_INCLUDE_DIR}
)

target_link_libraries(${target}
    d3d11
    d3d12
    d3dcompiler
    dxgi
    Utilities
    spirv-cross-core
    spirv-cross-hlsl
    glad
    Resource
    View
)

if (VULKAN_SUPPORT)
    target_link_libraries(${target}
        ${Vulkan_LIBRARIES}
    )
endif()

set_target_properties(${target} PROPERTIES FOLDER "library")
