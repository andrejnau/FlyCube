set(target Texture)

set(headers
    TextureLoader.h
    TextureInfo.h
    TextureCache.h
    FormatHelper.h
)

set(sources
    TextureLoader.cpp
    TextureCache.cpp
    FormatHelper.cpp
)

if (DIRECTX_SUPPORT)
    set(headers 
        ${headers}
        DXGIFormatHelper.h
    )
    set(sources 
        ${sources}
        DXGIFormatHelper.cpp
    )
endif()

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(${target}
    SOIL
    Utilities
    Context
)

set_target_properties(${target} PROPERTIES FOLDER "library")
