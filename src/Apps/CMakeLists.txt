if (NOT APPLE)
    add_subdirectory(CoreDxrTriangle)
endif()

if (NOT IOS_OR_TVOS)
    add_subdirectory(CoreTriangle)
endif()

if (CMAKE_GENERATOR MATCHES "Xcode")
    add_subdirectory(CoreTriangleUIKit)
endif()
