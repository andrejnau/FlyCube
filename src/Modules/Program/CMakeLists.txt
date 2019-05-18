set(target Program)

set(headers
    BufferLayout.h
    DX11ProgramApi.h
    DX12ProgramApi.h
    Program.h
    ProgramApi.h
    CommonProgramApi.h
    DX11ViewCreater.h
    DX12ViewCreater.h
    IShaderBlobProvider.h
    GLProgramApi.h
    DX11ViewDescCreater.h
    DX12ViewDescCreater.h
)

set(sources
    ProgramApi.cpp
    DX11ProgramApi.cpp
    DX12ProgramApi.cpp
    GLProgramApi.cpp
    CommonProgramApi.cpp
    DX11ViewCreater.cpp
    DX12ViewCreater.cpp
    DX11ViewDescCreater.cpp
    DX12ViewDescCreater.cpp
)

if (VULKAN_SUPPORT)
    set(headers 
        ${headers}
        VKProgramApi.h
        VKViewCreater.h
    )
    set(sources 
        ${sources}
        VKProgramApi.cpp
        VKViewCreater.cpp
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
    d3dcompiler
    Utilities
    spirv-cross-core
    spirv-cross-hlsl
    glLoadGen
    Shader
)

if (VULKAN_SUPPORT)
    target_link_libraries(${target}
        ${Vulkan_LIBRARIES}
    )
endif()

set_target_properties(${target} PROPERTIES FOLDER "library")
