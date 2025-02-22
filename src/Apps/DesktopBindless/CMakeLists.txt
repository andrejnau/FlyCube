add_executable(DesktopBindless WIN32 MACOSX_BUNDLE main.cpp)

if (WIN32)
    set_target_properties(DesktopBindless PROPERTIES
        LINK_FLAGS "/ENTRY:mainCRTStartup"
    )
endif()

target_link_libraries(DesktopBindless
    AppBox
    FlyCube
    FlyCubeAssets
)

set_target_properties(DesktopBindless PROPERTIES FOLDER "Apps")
