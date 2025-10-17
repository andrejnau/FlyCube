list(APPEND headers
    Model.h
    ModelLoader.h
    RenderModel.h
)

list(APPEND sources
    ModelLoader.cpp
    RenderModel.cpp
)

add_library(RenderUtils ${headers} ${sources})

target_include_directories(RenderUtils
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(RenderUtils
    assimp
    FlyCube
    FlyCubeAssets
    glm
    stb_image
)

set_target_properties(RenderUtils PROPERTIES FOLDER "Modules")
