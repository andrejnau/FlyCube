if (IOS_OR_TVOS)
    return()
endif()

add_library(AppBox STATIC
    $<$<BOOL:${APPLE}>:AutoreleasePool.h>
    $<$<BOOL:${APPLE}>:AutoreleasePool.mm>
    AppBox.cpp
    AppBox.h
    InputEvents.h
    WindowEvents.h
    WindowUtils.cpp
    WindowUtils.h
)

target_link_libraries(AppBox
    AppSettings
    FlyCube
    glfw
)

if (APPLE)
    set_property(SOURCE AutoreleasePool.mm PROPERTY COMPILE_FLAGS "-fno-objc-arc")
    set_property(SOURCE WindowUtils.cpp PROPERTY COMPILE_FLAGS "-xobjective-c++")
    target_link_libraries(AppBox
        "-framework Foundation"
        "-framework QuartzCore"
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
