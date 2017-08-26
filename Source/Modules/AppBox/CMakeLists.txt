add_library(AppBox STATIC
    AppBox.h
    AppBox.cpp
)

target_link_libraries(AppBox
    ISample
    glfw
)

set_target_properties(AppBox PROPERTIES FOLDER "library")
