set(target Scene)

set(headers
    IScene.h
    SceneBase.h
	ApiType.h
    Settings.h
)

set(sources
    SceneBase.cpp
    Settings.cpp
)

add_library(${target} ${headers} ${sources})

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(${target}
    Camera
)

set_target_properties(${target} PROPERTIES FOLDER "library")
