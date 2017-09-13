set (target OpenGLApp)

set (HEADERS
    scene.h
    shaders.h
    GLGeometry.h
)

set (SOURCES
    scene.cpp
    main.cpp
)

add_executable(${target} WIN32 ${HEADERS} ${SOURCES})
if (WIN32)
    set_target_properties(${target} PROPERTIES
                        LINK_FLAGS "/ENTRY:mainCRTStartup")
endif()

target_link_libraries(${target}
    AppBox
    gl_load
    SOIL
    assimp
    simple_camera
    utils
    ISample
    Geometry
    Utilities
)
