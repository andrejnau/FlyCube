set(target Shader)

set(headers
    ShaderBase.h
    DXCompiler.h
    SpirvCompiler.h
    GLSLConverter.h
)

set(sources
    DXCompiler.cpp
    SpirvCompiler.cpp
    GLSLConverter.cpp
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
	glLoadGen
)

set_target_properties(${target} PROPERTIES FOLDER "library")
