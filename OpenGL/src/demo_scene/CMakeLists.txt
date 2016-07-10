set (target demo_scene)

set (HEADERS
    scene.h
    geometry.h
    shaders.h)

set (SOURCES
    scene.cpp
    geometry.cpp)

add_library(${target} ${SOURCES} ${HEADERS})

if (ANDROID)
    target_link_libraries(${target} android log GLESv3 SOIL assimp simple_camera utils)
else()
    target_link_libraries(${target} gl_load SOIL assimp simple_camera utils)
endif()

target_include_directories(${target}
    PUBLIC
        .
)

set_target_properties(${target} PROPERTIES FOLDER "library")