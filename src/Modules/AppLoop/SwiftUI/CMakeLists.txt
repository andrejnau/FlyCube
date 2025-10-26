enable_language(Swift)

if (CMAKE_Swift_COMPILER_VERSION VERSION_LESS 5.9)
   message(FATAL_ERROR "Bidirectional C++ Interop requires Swift 5.9 or greater. Have ${CMAKE_Swift_COMPILER_VERSION}")
endif()

list(APPEND headers
   MetalRenderer.h
)

list(APPEND sources
   AppLoop.mm
   ContentView.swift
   MetalRenderer.mm
   MetalView.swift
)

if (CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "tvOS")
   list(APPEND sources
      AppLoopUI.swift
   )
else()
   list(APPEND sources
      AppLoopNS.swift
   )
endif()

add_library(AppLoopSwiftUI
   ${headers}
   ${sources}
)

set_target_properties(AppLoopSwiftUI PROPERTIES
   XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/SwiftBridgingHeader.h"
   XCODE_ATTRIBUTE_SWIFT_OBJC_INTERFACE_HEADER_NAME "SwiftInterfaceHeader.h"
)

target_include_directories(AppLoopSwiftUI
   PUBLIC
      "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)

target_link_libraries(AppLoopSwiftUI
   "-framework Metal"
   "-framework QuartzCore"
   AppSettings
)

if (CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "tvOS")
   target_link_libraries(AppLoopSwiftUI
      "-framework UIKit"
   )
else()
   target_link_libraries(AppLoopSwiftUI
      "-framework AppKit"
   )
endif()

set_target_properties(AppLoopSwiftUI PROPERTIES FOLDER "Modules/AppLoop")
