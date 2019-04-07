set(target Context)

set(headers
    BaseTypes.h
    Context.h
    ContextSelector.h
    DescriptorPool.h
    DX11Context.h
    DX12Context.h
    DXGIUtility.h
    VKContext.h
    VKDescriptorPool.h
    GLContext.h
)

set(sources
    Context.cpp
    ContextSelector.cpp
    DescriptorPool.cpp
    DX11Context.cpp
    DX12Context.cpp
    DXGIUtility.cpp
    VKContext.cpp
    VKDescriptorPool.cpp
    GLContext.cpp
)

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
    ${Vulkan_LIBRARIES}
    glLoadGen
    Resource
    View
)

set_target_properties(${target} PROPERTIES FOLDER "library")
