add_library(AppBox STATIC
    AppBox.h
    AppBox.cpp
)

target_include_directories(AppBox
    PUBLIC
        ${project_root}/Source
)

target_link_libraries(AppBox
    glfw
)
