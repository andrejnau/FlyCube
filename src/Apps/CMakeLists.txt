add_subdirectory(CoreTriangle)
if (RAYTRACING_SUPPORT)
    add_subdirectory(CoreDxrTriangle)
endif()

if (RAYTRACING_SUPPORT)
    add_subdirectory(DxrTriangle)
endif()

add_subdirectory(MeshShading)
add_subdirectory(ShaderParser)
add_subdirectory(SponzaPbr)
add_subdirectory(Triangle)
add_subdirectory(VariableRateShading)
