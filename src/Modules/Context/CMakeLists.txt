set(target Context)

set(headers
    BaseTypes.h
    Context.h
    ContextSelector.h
    DescriptorPool.h
    DX11Context.h
    DX11Resource.h
    DX12Context.h
    DX12Resource.h
    DXGIUtility.h
    Resource.h
    VKContext.h
)

set(sources
    Context.cpp
    DescriptorPool.cpp
    DX11Context.cpp
    DX12Context.cpp
    DX12Resource.cpp
    DXGIUtility.cpp
    VKContext.cpp
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
    ${Vulkan_LIBRARIES}
)

set_target_properties(${target} PROPERTIES FOLDER "library")
