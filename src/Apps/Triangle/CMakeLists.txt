list(APPEND sources
   main.mm
)

list(APPEND resources
   PixelShader.spv
   VertexShader.spv
)

set_source_files_properties(${resources} PROPERTIES
   MACOSX_PACKAGE_LOCATION Resources
)

add_executable(Triangle MACOSX_BUNDLE
   ${sources}
   ${resources}
)

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
