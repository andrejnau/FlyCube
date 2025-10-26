add_executable(DesktopDxrTriangle WIN32 MACOSX_BUNDLE main.cpp)

target_link_options(DesktopDxrTriangle
    PRIVATE
        $<$<BOOL:${WIN32}>:/ENTRY:mainCRTStartup>
)

target_link_libraries(DesktopDxrTriangle
    AppBox
    FlyCube
    FlyCubeAssets
)

set_target_properties(DesktopDxrTriangle PROPERTIES
    FOLDER "Apps"
    MACOSX_BUNDLE_INFO_PLIST "${project_root}/cmake/apple/Info.plist"
)
