set(target Texture)

set(headers
    TextureLoader.h
    TextureInfo.h
    TextureCache.h
)

set(sources
    TextureLoader.cpp
    TextureCache.cpp
)

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(${target}
    Utilities
    RenderDevice
)

set_target_properties(${target} PROPERTIES FOLDER "Modules")
