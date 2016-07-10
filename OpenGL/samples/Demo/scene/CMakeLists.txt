set (PROJECT DemoScene)
project (${PROJECT})

set (HEADERS
    scene.h
    geometry.h
    shaders.h)

set (SOURCES
    scene.cpp
    geometry.cpp)

add_library(${PROJECT} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT} glfw ${GLFW_LIBRARIES} SOIL gl_load assimp simple_camera utils)

set_target_properties(${PROJECT} PROPERTIES FOLDER "samples")

target_include_directories(${PROJECT}
    PUBLIC
        .
)