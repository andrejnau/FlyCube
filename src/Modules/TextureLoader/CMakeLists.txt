set(target TextureLoader)

set(headers
    GLTextureLoader.h
    DX11TextureLoader.h
    Texture.h
)

set(sources
    GLTextureLoader.cpp
    DX11TextureLoader.cpp
)

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(${target}
    SOIL
    d3d11
    gl_load
    Utilities
    D3DUtils
)

set_target_properties(${target} PROPERTIES FOLDER "library")
