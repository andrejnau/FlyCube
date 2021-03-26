list(APPEND headers
    RenderCommandList.h
    RenderCommandListImpl.h
)

list(APPEND sources
    RenderCommandListImpl.cpp
)

add_library(RenderCommandList ${headers} ${sources})

target_include_directories(RenderCommandList
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(RenderCommandList
    ApiType
    Swapchain
    Shader
    CommandList
    Program
    Framebuffer
    Pipeline
    GPUDescriptorPool
    Fence
    Utilities
)

set_target_properties(RenderCommandList PROPERTIES FOLDER "Render")
