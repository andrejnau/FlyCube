list(APPEND sources
   main.cpp
)

set(shaders_path "${assets_path}/shaders/DispatchIndirect")

set(shaders_files ${shaders_path}/ComputeShader.hlsl)
set_property(SOURCE ${shaders_files} PROPERTY SHADER_TYPE Compute)
set_property(SOURCE ${shaders_files} PROPERTY SHADER_ENTRYPOINT main)
set_property(SOURCE ${shaders_files} PROPERTY SHADER_MODEL 6_0)

include(compile_shaders)
compile_shaders("${shaders_path}" "assets/DispatchIndirect" "${shaders_files}" resources)

if (ANDROID)
   add_library(DispatchIndirect SHARED
      ${resources}
      ${shaders_files}
      ${sources}
   )
else()
   add_executable(DispatchIndirect WIN32 MACOSX_BUNDLE
      ${resources}
      ${shaders_files}
      ${sources}
   )
endif()

target_link_options(DispatchIndirect
    PRIVATE
        $<$<BOOL:${WIN32}>:/ENTRY:mainCRTStartup>
)

target_link_libraries(DispatchIndirect
   AppLoop
   FlyCube
)

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
   set_target_properties(DispatchIndirect PROPERTIES
      XCODE_EXPLICIT_FILE_TYPE "wrapper.application"
   )
endif()

set_target_properties(DispatchIndirect PROPERTIES
    FOLDER "Apps"
    MACOSX_BUNDLE_INFO_PLIST "${project_root}/cmake/apple/Info.plist"
)
