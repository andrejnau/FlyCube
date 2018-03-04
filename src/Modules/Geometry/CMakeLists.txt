set(target Geometry)

set(headers
    IMesh.h
    IModel.h
    Mesh.h
    Bones.h
    Model.h
    ModelLoader.h
    Geometry.h
)

set(sources
    Mesh.cpp
    Bones.cpp
    Model.cpp
    ModelLoader.cpp
)

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(${target}
    Utilities
    assimp
)

set_target_properties(${target} PROPERTIES FOLDER "library")
