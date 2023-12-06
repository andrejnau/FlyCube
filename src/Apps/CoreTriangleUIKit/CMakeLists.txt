list(APPEND sources
   main.mm
)

list(APPEND resources
   PixelShader.spv
   VertexShader.spv
)

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
   list(APPEND resources
      iOS/Base.lproj/Main.storyboard
   )
elseif (CMAKE_SYSTEM_NAME STREQUAL "tvOS")
   list(APPEND resources
      tvOS/Base.lproj/Main.storyboard
   )
endif()

set_source_files_properties(${resources} PROPERTIES
   MACOSX_PACKAGE_LOCATION Resources
)

add_executable(CoreTriangleUIKit MACOSX_BUNDLE
   ${sources}
   ${resources}
)

target_link_libraries(CoreTriangleUIKit
   AppBoxUIKit
)

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
   set_target_properties(CoreTriangleUIKit PROPERTIES
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/iOS/Info.plist"
   )
elseif (CMAKE_SYSTEM_NAME STREQUAL "tvOS")
   set_target_properties(CoreTriangleUIKit PROPERTIES
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/tvOS/Info.plist"
   )
endif()

set_target_properties(CoreTriangleUIKit PROPERTIES FOLDER "Apps")
