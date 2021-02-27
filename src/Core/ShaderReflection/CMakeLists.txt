list(APPEND headers
    ShaderReflection.h
)
list(APPEND sources
    ShaderReflection.cpp
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        SPIRVReflection.h
    )
    list(APPEND sources
        SPIRVReflection.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXILReflection.h
    )
    list(APPEND sources
        DXILReflection.cpp
    )
endif()

add_library(ShaderReflection ${headers} ${sources})

target_include_directories(ShaderReflection
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(ShaderReflection
    Utilities
    HLSLCompiler
)

if (VULKAN_SUPPORT)
    target_link_libraries(ShaderReflection
        spirv-cross-core
        spirv-cross-hlsl
    )
endif()

if (DIRECTX_SUPPORT)
    find_package(DiaSDK REQUIRED)
    target_include_directories(ShaderReflection
        PRIVATE
            ${DIASDK_INCLUDE_DIRS}
    )
    target_link_libraries(ShaderReflection
        d3d12
        dxgi
        dxguid
    )
    target_include_directories(ShaderReflection
        PUBLIC
            "${project_root}/3rdparty/unpacked/d3dx12"
    )
endif()

set_target_properties(ShaderReflection PROPERTIES FOLDER "Core")

add_subdirectory(test)
