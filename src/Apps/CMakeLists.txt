add_subdirectory(ParseShader)
add_subdirectory(Triangle)
add_subdirectory(SponzaPbr)

if (RAYTRACING_SUPPORT)
    add_subdirectory(DxrTriangle)
endif()
