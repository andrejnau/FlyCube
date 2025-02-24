add_executable(DesktopDxrTriangle WIN32 MACOSX_BUNDLE main.cpp)

if (WIN32)
    set_target_properties(DesktopDxrTriangle PROPERTIES
        LINK_FLAGS "/ENTRY:mainCRTStartup"
    )
endif()

target_link_libraries(DesktopDxrTriangle
    AppBox
    FlyCube
    FlyCubeAssets
)

set_target_properties(DesktopDxrTriangle PROPERTIES FOLDER "Apps")
