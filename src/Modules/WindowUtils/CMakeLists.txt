add_library(WindowUtils
    WindowUtils.cpp
    WindowUtils.h
)

if (APPLE)
    set_property(SOURCE WindowUtils.cpp PROPERTY COMPILE_FLAGS "-xobjective-c++")
endif()

target_link_libraries(WindowUtils
    glfw
)

target_include_directories(WindowUtils
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

set_target_properties(WindowUtils PROPERTIES FOLDER "Modules")
