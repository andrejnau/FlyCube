set(target Context)

set(headers
    Context.h
    DX11Context.h
    DX12Context.h
    ContextSelector.h
)

set(sources
    Context.cpp
    DescriptorPool.cpp
    DX11Context.cpp
    DX12Context.cpp
)

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        "${project_root}/3rdparty/d3dx12"
)

target_link_libraries(${target}
    Utilities
    d3dcompiler
    d3d11
    d3d12
    dxgi
)

set_target_properties(${target} PROPERTIES FOLDER "library")
