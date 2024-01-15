add_library(AppLoopGLFW AppLoop.cpp)

if (APPLE)
    set_property(SOURCE AppLoop.cpp PROPERTY COMPILE_FLAGS "-xobjective-c++")
endif()

target_link_libraries(AppLoopGLFW
    glfw
)

target_include_directories(AppLoopGLFW
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)

set_target_properties(AppLoopGLFW PROPERTIES FOLDER "Modules/AppLoop")
