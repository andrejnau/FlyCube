list(APPEND sources
   main.cpp
)

set(shaders_path "${assets_path}/shaders/RayTracingTriangle")

set(library_shaders
    ${shaders_path}/RayTracing.hlsl
    ${shaders_path}/RayTracingCallable.hlsl
    ${shaders_path}/RayTracingHit.hlsl
)
set_property(SOURCE ${library_shaders} PROPERTY SHADER_TYPE Library)

set(compute_shaders
    ${shaders_path}/RayQuery.hlsl
)
set_property(SOURCE ${compute_shaders} PROPERTY SHADER_TYPE Compute)

set(shaders_files ${library_shaders} ${compute_shaders})
set_property(SOURCE ${shaders_files} PROPERTY SHADER_ENTRYPOINT main)
set_property(SOURCE ${shaders_files} PROPERTY SHADER_MODEL 6_5)

include(compile_shaders)
compile_shaders("${shaders_path}" "assets/RayTracingTriangle" "${shaders_files}" resources)

if (ANDROID)
   add_library(RayTracingTriangle SHARED
      ${resources}
      ${shaders_files}
      ${sources}
   )
else()
   add_executable(RayTracingTriangle WIN32 MACOSX_BUNDLE
      ${resources}
      ${shaders_files}
      ${sources}
   )
endif()

target_link_options(RayTracingTriangle
    PRIVATE
        $<$<BOOL:${WIN32}>:/ENTRY:mainCRTStartup>
)

target_link_libraries(RayTracingTriangle
   AppLoop
   FlyCube
)

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
   set_target_properties(RayTracingTriangle PROPERTIES
      XCODE_EXPLICIT_FILE_TYPE "wrapper.application"
   )
endif()

set_target_properties(RayTracingTriangle PROPERTIES
    FOLDER "Apps"
    MACOSX_BUNDLE_INFO_PLIST "${project_root}/cmake/apple/Info.plist"
)
