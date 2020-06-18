list(APPEND headers
    CommandListBox.h
)

list(APPEND sources
    CommandListBox.cpp
)

add_library(CommandListBox ${headers} ${sources})

target_include_directories(CommandListBox
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(CommandListBox
    ApiType
    Swapchain
    Shader
    CommandList
    Program
    Framebuffer
    Pipeline
    CPUDescriptorPool
    GPUDescriptorPool
    Fence
    Utilities
    spirv-cross-core
    spirv-cross-hlsl
)

set_target_properties(CommandListBox PROPERTIES FOLDER "Modules")
