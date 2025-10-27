list(APPEND sources
   main.cpp
)

set(shaders_path "${assets_path}/shaders/ModelView")

set(vertex_shaders ${shaders_path}/VertexShader.hlsl)
set_property(SOURCE ${vertex_shaders} PROPERTY SHADER_TYPE Vertex)
set(pixel_shaders ${shaders_path}/PixelShader.hlsl)
set_property(SOURCE ${pixel_shaders} PROPERTY SHADER_TYPE Pixel)

set(shaders_files ${vertex_shaders} ${pixel_shaders})
set_property(SOURCE ${shaders_files} PROPERTY SHADER_ENTRYPOINT main)
set_property(SOURCE ${shaders_files} PROPERTY SHADER_MODEL 6_0)

include(compile_shaders)
compile_shaders("${shaders_path}" "assets/ModelView" "${shaders_files}" resources)

list(APPEND damaged_helmet
    "${assets_path}/resources/models/DamagedHelmet/glTF/DamagedHelmet.bin"
    "${assets_path}/resources/models/DamagedHelmet/glTF/DamagedHelmet.gltf"
    "${assets_path}/resources/models/DamagedHelmet/glTF/Default_albedo.jpg"
    "${assets_path}/resources/models/DamagedHelmet/glTF/Default_AO.jpg"
    "${assets_path}/resources/models/DamagedHelmet/glTF/Default_emissive.jpg"
    "${assets_path}/resources/models/DamagedHelmet/glTF/Default_metalRoughness.jpg"
    "${assets_path}/resources/models/DamagedHelmet/glTF/Default_normal.jpg"
)
set_source_files_properties(${damaged_helmet} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources/assets/resources/models/DamagedHelmet/glTF"
)
list(APPEND resources
    ${damaged_helmet}
)

if (ANDROID)
   add_library(DesktopModelView SHARED
      ${resources}
      ${shaders_files}
      ${sources}
   )
else()
   add_executable(DesktopModelView WIN32 MACOSX_BUNDLE
      ${resources}
      ${shaders_files}
      ${sources}
   )
endif()

target_link_options(DesktopModelView
    PRIVATE
        $<$<BOOL:${WIN32}>:/ENTRY:mainCRTStartup>
)

target_link_libraries(DesktopModelView
    AppBox
    FlyCube
    FlyCubeAssets
    RenderUtils
)

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
   set_target_properties(DesktopModelView PROPERTIES
      XCODE_EXPLICIT_FILE_TYPE "wrapper.application"
   )
endif()

set_target_properties(DesktopModelView PROPERTIES
    FOLDER "Apps"
    MACOSX_BUNDLE_INFO_PLIST "${project_root}/cmake/apple/Info.plist"
)
