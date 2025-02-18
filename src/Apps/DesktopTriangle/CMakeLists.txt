add_executable(DesktopTriangle WIN32 main.cpp)

if (WIN32)
    set_target_properties(DesktopTriangle PROPERTIES
        LINK_FLAGS "/ENTRY:mainCRTStartup"
    )
endif()

target_link_libraries(DesktopTriangle
    AppBox
    FlyCube
    FlyCubeAssets
)

set_target_properties(DesktopTriangle PROPERTIES FOLDER "Apps")

install(TARGETS DesktopTriangle)
