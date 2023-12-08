enable_language(Swift)

list(APPEND headers
   AppBox.h
   MetalRenderer.h
)

list(APPEND sources
   AppBox.mm
   ContentView.swift
   MetalRenderer.mm
   MetalView.swift
)

if (CMAKE_SYSTEM_NAME STREQUAL "iOS" OR MAKE_SYSTEM_NAME STREQUAL "tvOS")
   list(APPEND sources
      AppDelegateUI.swift
   )
else()
   list(APPEND sources
      AppDelegateNS.swift
   )
endif()

add_library(AppBoxUIKit
   ${headers}
   ${sources}
)

set_target_properties(AppBoxUIKit PROPERTIES
   XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/SwiftBridgingHeader.h"
   XCODE_ATTRIBUTE_SWIFT_OBJC_INTERFACE_HEADER_NAME "AppBoxUIKitSwift.h"
)

target_include_directories(AppBoxUIKit
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(AppBoxUIKit
   "-framework Metal"
   "-framework QuartzCore"
)

if (CMAKE_SYSTEM_NAME STREQUAL "iOS" OR MAKE_SYSTEM_NAME STREQUAL "tvOS")
   target_link_libraries(AppBoxUIKit
      "-framework UIKit"
   )
else()
   target_link_libraries(AppBoxUIKit
      "-framework AppKit"
   )
endif()

set_target_properties(AppBoxUIKit PROPERTIES FOLDER "Modules")
