add_library(AppLoopAndroid
    AppLoop.cpp
)

find_package(game-activity REQUIRED CONFIG)

target_link_libraries(AppLoopAndroid
    android
    AppSettings
    FlyCube
    game-activity::game-activity_static
    log
)

target_link_options(AppLoopAndroid
    PUBLIC
        -u Java_com_google_androidgamesdk_GameActivity_initializeNativeCode
)

target_include_directories(AppLoopAndroid
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)

set_target_properties(AppLoopAndroid PROPERTIES FOLDER "Modules/AppLoop")
