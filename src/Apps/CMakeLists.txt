if (NOT APPLE)
    add_subdirectory(CoreDxrTriangle)
endif()

if (IOS_OR_TVOS)
    add_subdirectory(CoreTriangleUIKit)
else()
    add_subdirectory(CoreTriangle)
endif()
