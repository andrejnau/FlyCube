import SwiftUI

struct AppLoop: App {
    var body: some Scene {
        WindowGroup {
            ContentView()
        }
    }
}

@_cdecl("swift_main")
public func swift_main() {
    AppLoop.main()
}
