list(APPEND headers
    PipelineProgram.h
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKPipelineProgram.h
    )
    list(APPEND sources
        VKPipelineProgram.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXPipelineProgram.h
    )
    list(APPEND sources
        DXPipelineProgram.cpp
    )
endif()

add_library(PipelineProgram ${headers} ${sources})

target_include_directories(PipelineProgram
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(PipelineProgram
    ApiType
    Device
    Utilities
    Shader
    spirv-cross-core
    spirv-cross-hlsl
    glad
)

if (VULKAN_SUPPORT)
    target_link_libraries(PipelineProgram
        ${Vulkan_LIBRARIES}
    )
    target_include_directories(PipelineProgram
        PUBLIC
            ${Vulkan_INCLUDE_DIR}
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(PipelineProgram
        d3d11
        d3d12
        dxgi
        dxguid
    )
    target_include_directories(PipelineProgram
        PUBLIC
            "${project_root}/3rdparty/d3dx12"
    )
endif()

set_target_properties(PipelineProgram PROPERTIES FOLDER "FlyEngine")
