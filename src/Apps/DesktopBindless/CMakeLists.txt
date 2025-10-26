add_executable(DesktopBindless WIN32 MACOSX_BUNDLE main.cpp)

target_link_options(DesktopBindless
    PRIVATE
        $<$<BOOL:${WIN32}>:/ENTRY:mainCRTStartup>
)

target_link_libraries(DesktopBindless
    AppBox
    FlyCube
    FlyCubeAssets
)

set_target_properties(DesktopBindless PROPERTIES
    FOLDER "Apps"
    MACOSX_BUNDLE_INFO_PLIST "${project_root}/cmake/apple/Info.plist"
)
