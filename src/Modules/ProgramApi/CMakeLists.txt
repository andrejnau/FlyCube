set(target ProgramApi)

set(headers
    BufferLayout.h
    Program.h
    ProgramApi.h
)

set(sources
    ProgramApi.cpp
)

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        "${project_root}/3rdparty/d3dx12"
        ${Vulkan_INCLUDE_DIR}
)

target_link_libraries(${target}
    Utilities
    spirv-cross-core
    spirv-cross-hlsl
    glad
    Shader
    Context
)

if (VULKAN_SUPPORT)
    target_link_libraries(${target}
        ${Vulkan_LIBRARIES}
    )
endif()

set_target_properties(${target} PROPERTIES FOLDER "Modules")
