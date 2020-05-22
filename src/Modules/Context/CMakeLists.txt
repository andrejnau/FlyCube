list(APPEND headers
    Context.h
)

list(APPEND sources
    Context.cpp
)

add_library(Context ${headers} ${sources})

target_include_directories(Context
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        "${project_root}/3rdparty/d3dx12"
        ${Vulkan_INCLUDE_DIR}
)

target_link_libraries(Context
    ApiType
    Instance
    Utilities
    spirv-cross-core
    spirv-cross-hlsl
    glad
    Resource
    ProgramApi
    View
)

if (VULKAN_SUPPORT)
    target_link_libraries(Context
        ${Vulkan_LIBRARIES}
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(Context
        d3d11
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(Context PROPERTIES FOLDER "Modules")
