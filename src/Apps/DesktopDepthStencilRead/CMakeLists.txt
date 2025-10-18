add_executable(DesktopDepthStencilRead WIN32 MACOSX_BUNDLE main.cpp)

target_link_options(DesktopDepthStencilRead
    PRIVATE
        $<$<BOOL:${WIN32}>:/ENTRY:mainCRTStartup>
)

target_link_libraries(DesktopDepthStencilRead
    AppBox
    FlyCube
    FlyCubeAssets
    RenderUtils
)

set_target_properties(DesktopDepthStencilRead PROPERTIES FOLDER "Apps")
