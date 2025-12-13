enable_language(Swift)

if (CMAKE_Swift_COMPILER_VERSION VERSION_LESS 5.9)
    message(FATAL_ERROR "Bidirectional C++ Interop requires Swift 5.9 or greater. Have ${CMAKE_Swift_COMPILER_VERSION}")
endif()

add_library(AppLoopSwiftUI STATIC
    AppLoop.mm
    AppLoop.swift
    ContentView.swift
    MetalRenderer.h
    MetalRenderer.mm
    MetalView.swift
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
    "-framework MetalKit"
    AppSettings
)

if (NOT IOS_OR_TVOS)
    target_link_libraries(AppLoopSwiftUI
        "-framework AppKit"
    )
endif()

set_target_properties(AppLoopSwiftUI PROPERTIES FOLDER "Modules/AppLoop")
