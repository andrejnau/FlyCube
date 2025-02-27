add_library(AppLoop INTERFACE)

if (CMAKE_GENERATOR MATCHES "Xcode")
    add_subdirectory(SwiftUI)
    target_link_libraries(AppLoop
        INTERFACE
            AppLoopSwiftUI
    )
elseif (ANDROID)
    add_subdirectory(Android)
    target_link_libraries(AppLoop
        INTERFACE
            AppLoopAndroid
    )
else()
    add_subdirectory(GLFW)
    target_link_libraries(AppLoop
        INTERFACE
            AppLoopGLFW
    )
endif()

target_include_directories(AppLoop
    INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

set_target_properties(AppLoop PROPERTIES FOLDER "Modules")
