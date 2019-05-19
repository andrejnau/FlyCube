set(target Texture)

set(headers
    TextureLoader.h
    TextureInfo.h
    DXGIFormatHelper.h
    TextureCache.h
)

set(sources
    TextureLoader.cpp
    DXGIFormatHelper.cpp
    TextureCache.cpp
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
