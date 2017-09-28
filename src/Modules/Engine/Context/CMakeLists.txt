set(target Context)

set(headers
    Context.h
)

set(sources
    Context.cpp
)

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(${target}
    FrameBuffer
    Geometry
    Program
    SwapChain
    Texture
)

if (BUILD_DX11)
    target_link_libraries(${target}
        DX11Context
    )
endif()

if (BUILD_OPENGL)
    target_link_libraries(${target}
        #GLContext
    )
endif()

set_target_properties(${target} PROPERTIES FOLDER "library/Engine")
