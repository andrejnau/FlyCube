add_executable(DesktopModelView WIN32 MACOSX_BUNDLE main.cpp)

target_link_options(DesktopModelView
    PRIVATE
        $<$<BOOL:${WIN32}>:/ENTRY:mainCRTStartup>
)

target_link_libraries(DesktopModelView
    AppBox
    FlyCube
    FlyCubeAssets
    RenderUtils
)

set_target_properties(DesktopModelView PROPERTIES FOLDER "Apps")
