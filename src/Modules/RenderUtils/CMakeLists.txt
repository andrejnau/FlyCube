add_library(RenderUtils STATIC
    Model.h
    ModelLoader.cpp
    ModelLoader.h
    RenderModel.cpp
    RenderModel.h
)

target_include_directories(RenderUtils
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(RenderUtils
    assimp
    FlyCube
    glm
    stb_image
)

set_target_properties(RenderUtils PROPERTIES FOLDER "Modules")
