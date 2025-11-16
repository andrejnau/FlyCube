add_library(AppLoopGLFW STATIC
    AppLoop.cpp
)

target_link_libraries(AppLoopGLFW
    AppBox
)

target_include_directories(AppLoopGLFW
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)

set_target_properties(AppLoopGLFW PROPERTIES FOLDER "Modules/AppLoop")
