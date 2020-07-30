add_subdirectory(ShaderParser)
add_subdirectory(Triangle)
add_subdirectory(Example)

add_subdirectory(SponzaPbr)

if (RAYTRACING_SUPPORT)
    add_subdirectory(DxrTriangle)
endif()

add_subdirectory(ModelViewer)
