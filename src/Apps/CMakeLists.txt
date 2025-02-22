if (NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(DesktopBindless)
    add_subdirectory(DesktopDxrTriangle)
    add_subdirectory(DesktopTriangle)
endif()

add_subdirectory(MeshTriangle)
add_subdirectory(Triangle)
