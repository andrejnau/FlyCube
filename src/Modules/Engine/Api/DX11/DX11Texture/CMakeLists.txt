set(target DX11Texture)

set(headers
    DX11TextureLoader.h
    TextureInfo.h
    DXGIFormatHelper.h
)

set(sources
    DX11TextureLoader.cpp
    DXGIFormatHelper.cpp
)

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(${target}
    SOIL
    d3d11
    dxgi
    Utilities
)

set_target_properties(${target} PROPERTIES FOLDER "library")
