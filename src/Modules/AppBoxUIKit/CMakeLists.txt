enable_language(Swift)

list(APPEND headers
   AppBox.h
   MetalRenderer.h
)

list(APPEND sources
   AppBox.mm
   AppDelegate.swift
   ContentView.swift
   MetalRenderer.mm
   MetalView.swift
)

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
   "-framework UIKit"
)

set_target_properties(AppBoxUIKit PROPERTIES FOLDER "Modules")
