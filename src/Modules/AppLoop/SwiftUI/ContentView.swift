import SwiftUI

struct ContentView: View {
    var body: some View {
        MetalView(renderer: MetalRenderer()).edgesIgnoringSafeArea(.all)
    }
}
