import Cocoa
import SwiftUI

class AppDelegate: NSObject, NSApplicationDelegate {
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        return true
    }
}

struct AppLoop: App {
    @NSApplicationDelegateAdaptor(AppDelegate.self) var app_delegate
    
    var body: some Scene {
        WindowGroup {
            ContentView().navigationTitle(MetalRenderer.getAppTitle())
        }.defaultSize(GetDefaultSize())
    }
    
    func GetDefaultSize() -> NSSize {
        let screen_size = NSScreen.main!.frame.size
        return NSMakeSize(screen_size.width / 1.5, screen_size.height / 1.5)
    }
}

@_cdecl("swift_main")
public func swift_main() {
    AppLoop.main()
}
