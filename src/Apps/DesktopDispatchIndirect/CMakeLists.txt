add_executable(DesktopDispatchIndirect WIN32 MACOSX_BUNDLE main.cpp)

target_link_options(DesktopDispatchIndirect
    PRIVATE
        $<$<BOOL:${WIN32}>:/ENTRY:mainCRTStartup>
)

target_link_libraries(DesktopDispatchIndirect
    AppBox
    FlyCube
    FlyCubeAssets
)

set_target_properties(DesktopDispatchIndirect PROPERTIES FOLDER "Apps")
