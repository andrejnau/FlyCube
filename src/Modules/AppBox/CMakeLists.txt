if (IOS_OR_TVOS)
    return()
endif()

list(APPEND headers
    AppBox.h
    InputEvents.h
    WindowEvents.h
)

list(APPEND sources
    AppBox.cpp
)

if (APPLE)
    list(APPEND headers
        AutoreleasePool.h
    )
    list(APPEND sources
        AutoreleasePool.mm
    )
    set_property(SOURCE AutoreleasePool.mm PROPERTY COMPILE_FLAGS "-fno-objc-arc")
endif()

add_library(AppBox ${headers} ${sources})

target_link_libraries(AppBox
    AppSettings
    FlyCube
    glfw
    WindowUtils
)

if (APPLE)
    set_property(SOURCE AppBox.cpp PROPERTY COMPILE_FLAGS "-xobjective-c++")
    target_link_libraries(AppBox
        "-framework Foundation"
    )
endif()

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
