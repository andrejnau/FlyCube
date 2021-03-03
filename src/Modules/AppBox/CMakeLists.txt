list(APPEND headers
    AppBox.h
    ArgsParser.h
    InputEvents.h
    Settings.h
    WindowEvents.h
)

list(APPEND sources
    AppBox.cpp
    ArgsParser.cpp
)

add_library(AppBox ${headers} ${sources})

target_link_libraries(AppBox
    glfw
    ApiType
)

if (VULKAN_SUPPORT)
    target_link_libraries(AppBox
        vulkan
    )
endif()

target_include_directories(AppBox
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

set_target_properties(AppBox PROPERTIES FOLDER "Modules")
