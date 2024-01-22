if (NOT APPLE)
    add_subdirectory(CoreDxrTriangle)
endif()

if (NOT IOS_OR_TVOS)
    add_subdirectory(CoreTriangle)
endif()

add_subdirectory(Triangle)
