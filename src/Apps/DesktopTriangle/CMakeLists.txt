add_executable(DesktopTriangle WIN32 MACOSX_BUNDLE main.cpp)

target_link_options(DesktopTriangle
    PRIVATE
        $<$<BOOL:${WIN32}>:/ENTRY:mainCRTStartup>
)

target_link_libraries(DesktopTriangle
    AppBox
    FlyCube
    FlyCubeAssets
)

set_target_properties(DesktopTriangle PROPERTIES
    FOLDER "Apps"
    MACOSX_BUNDLE_INFO_PLIST "${project_root}/cmake/apple/Info.plist"
)
