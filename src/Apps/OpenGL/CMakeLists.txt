set (target OpenGLApp)

set (HEADERS
    scene.h
    geometry.h
    shaders.h
)

set (SOURCES
    scene.cpp
    geometry.cpp
    main.cpp
)

add_executable(${target} WIN32 ${SOURCES})
set_target_properties(${target} PROPERTIES
                    LINK_FLAGS "/ENTRY:mainCRTStartup")

target_link_libraries(${target}
    AppBox
    gl_load
    SOIL
    assimp
    simple_camera
    utils
    ISample
)
