set(target Example)

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}")
set(source_path "${CMAKE_CURRENT_SOURCE_DIR}")

set(sources
    ${source_path}/main.cpp
)

add_executable(${target} WIN32 ${headers} ${sources})
if (WIN32)
    set_target_properties(${target} PROPERTIES
                            LINK_FLAGS "/ENTRY:mainCRTStartup")
endif()

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        ${CMAKE_BINARY_DIR}/gen/${target}
)

target_link_libraries(${target}
    Context
    assimp
    SOIL
    AppBox
    Scene
    Geometry
    Utilities
    Texture
    Program
    imgui
)
