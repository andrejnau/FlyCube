if (NOT APPLE)
    add_subdirectory(DesktopDxrTriangle)
endif()

if (NOT IOS_OR_TVOS)
    add_subdirectory(DesktopTriangle)
endif()

add_subdirectory(Triangle)
