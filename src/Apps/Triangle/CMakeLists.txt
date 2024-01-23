list(APPEND sources
   main.cpp
)

set(shaders_path "${assets_path}/shaders/CoreTriangle")

set(pixel_shaders
    ${shaders_path}/PixelShader.hlsl
)
set(vertex_shaders
    ${shaders_path}/VertexShader.hlsl
)

set_property(SOURCE ${vertex_shaders} ${pixel_shaders} PROPERTY SHADER_ENTRYPOINT main)
set_property(SOURCE ${vertex_shaders} ${pixel_shaders} PROPERTY SHADER_MODEL 6_0)
set_property(SOURCE ${vertex_shaders} PROPERTY SHADER_TYPE Vertex)
set_property(SOURCE ${pixel_shaders} PROPERTY SHADER_TYPE Pixel)

set(shaders_files ${pixel_shaders} ${vertex_shaders})

include(compile_shaders)
compile_shaders("${shaders_path}" "asserts/Triangle" "${shaders_files}" resources)

add_executable(Triangle WIN32 MACOSX_BUNDLE
   ${resources}
   ${shaders_files}
   ${sources}
)

if (WIN32)
    set_target_properties(Triangle PROPERTIES
        LINK_FLAGS "/ENTRY:mainCRTStartup"
    )
endif()

target_link_libraries(Triangle
   AppLoop
   AppSettings
   FlyCube
)

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
   set_target_properties(Triangle PROPERTIES
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/iOS/Info.plist"
   )
   set_target_properties(Triangle PROPERTIES
      XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # iPhone, iPad
   )
elseif (CMAKE_SYSTEM_NAME STREQUAL "tvOS")
   set_target_properties(Triangle PROPERTIES
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/tvOS/Info.plist"
   )
endif()

set_target_properties(Triangle PROPERTIES FOLDER "Apps")
