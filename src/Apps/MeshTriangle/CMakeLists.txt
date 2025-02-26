list(APPEND sources
   main.cpp
)

set(shaders_path "${assets_path}/shaders/MeshTriangle")

set(mesh_shaders ${shaders_path}/MeshShader.hlsl)
set_property(SOURCE ${mesh_shaders} PROPERTY SHADER_TYPE Mesh)
set(pixel_shaders ${shaders_path}/PixelShader.hlsl)
set_property(SOURCE ${pixel_shaders} PROPERTY SHADER_TYPE Pixel)

set(shaders_files ${mesh_shaders} ${pixel_shaders})
set_property(SOURCE ${shaders_files} PROPERTY SHADER_ENTRYPOINT main)
set_property(SOURCE ${shaders_files} PROPERTY SHADER_MODEL 6_5)

include(compile_shaders)
compile_shaders("${shaders_path}" "assets/MeshTriangle" "${shaders_files}" resources)

if (ANDROID)
   add_library(MeshTriangle SHARED
      ${resources}
      ${shaders_files}
      ${sources}
   )
else()
   add_executable(MeshTriangle WIN32 MACOSX_BUNDLE
      ${resources}
      ${shaders_files}
      ${sources}
   )
endif()

target_link_options(MeshTriangle
    PRIVATE
        $<$<BOOL:${WIN32}>:/ENTRY:mainCRTStartup>
)

target_link_libraries(MeshTriangle
   AppLoop
   AppSettings
   FlyCube
)

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
   set_target_properties(MeshTriangle PROPERTIES
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/iOS/Info.plist"
   )
   set_target_properties(MeshTriangle PROPERTIES
      XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # iPhone, iPad
   )
elseif (CMAKE_SYSTEM_NAME STREQUAL "tvOS")
   set_target_properties(MeshTriangle PROPERTIES
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/tvOS/Info.plist"
   )
endif()

set_target_properties(MeshTriangle PROPERTIES FOLDER "Apps")
