list(APPEND sources
   main.cpp
)

set(shaders_path "${assets_path}/shaders/Triangle")

set(vertex_shaders ${shaders_path}/VertexShader.hlsl)
set_property(SOURCE ${vertex_shaders} PROPERTY SHADER_TYPE Vertex)
set(pixel_shaders ${shaders_path}/PixelShaderNoBindings.hlsl)
set_property(SOURCE ${pixel_shaders} PROPERTY SHADER_TYPE Pixel)

set(shaders_files ${vertex_shaders} ${pixel_shaders})
set_property(SOURCE ${shaders_files} PROPERTY SHADER_ENTRYPOINT main)
set_property(SOURCE ${shaders_files} PROPERTY SHADER_MODEL 6_0)

include(compile_shaders)
compile_shaders("${shaders_path}" "assets/Triangle" "${shaders_files}" resources)

if (ANDROID)
   add_library(Triangle SHARED
      ${resources}
      ${shaders_files}
      ${sources}
   )
else()
   add_executable(Triangle WIN32 MACOSX_BUNDLE
      ${resources}
      ${shaders_files}
      ${sources}
   )
endif()

target_link_options(Triangle
    PRIVATE
        $<$<BOOL:${WIN32}>:/ENTRY:mainCRTStartup>
)

target_link_libraries(Triangle
   AppLoop
   AppSettings
   FlyCube
)

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
   set_target_properties(Triangle PROPERTIES
      MACOSX_BUNDLE_INFO_PLIST "${project_root}/cmake/apple/iOS/Info.plist"
   )
   set_target_properties(Triangle PROPERTIES
      XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # iPhone, iPad
   )
elseif (CMAKE_SYSTEM_NAME STREQUAL "tvOS")
   set_target_properties(Triangle PROPERTIES
      MACOSX_BUNDLE_INFO_PLIST "${project_root}/cmake/apple/tvOS/Info.plist"
   )
endif()

set_target_properties(Triangle PROPERTIES FOLDER "Apps")
