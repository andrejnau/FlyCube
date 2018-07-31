set(target Program)

set(headers
    BufferLayout.h
    DX11ProgramApi.h
    DX12ProgramApi.h
    Program.h
    ProgramApi.h
    ShaderBase.h
    DXProgram.h
    VKProgramApi.h
    CommonProgramApi.h
    CommonProgramApi.cpp
    DX12ViewCreater.h
    DX12ViewCreater.cpp
	IShaderBlobProvider.h
    VKViewCreater.h
    VKViewCreater.cpp
)

set(sources
    DX11ProgramApi.cpp
    DX12ProgramApi.cpp
    VKProgramApi.cpp
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
    d3dcompiler
    Utilities
	spirv-cross-core
	spirv-cross-hlsl
    ${Vulkan_LIBRARIES}
)

set_target_properties(${target} PROPERTIES FOLDER "library")
