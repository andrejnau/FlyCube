set (target OpenGLApp)

set (HEADERS
    GLScene.h
)

set (SOURCES
    GLScene.cpp
    main.cpp
)

add_executable(${target} WIN32 ${HEADERS} ${SOURCES})
if (WIN32)
    set_target_properties(${target}
        PROPERTIES
            LINK_FLAGS "/ENTRY:mainCRTStartup"
    )
endif()

target_link_libraries(${target}
    AppBox
    glLoadGen
    SOIL
    assimp
    Scene
    Geometry
    Utilities
    Texture
    Program
)
