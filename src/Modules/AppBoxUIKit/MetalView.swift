import SwiftUI
import MetalKit

struct MetalView: UIViewRepresentable {
    var renderer: MetalRenderer

    func makeUIView(context: Context) -> MTKView {
        let view = MTKView(frame: .zero, device: nil)
        view.delegate = renderer
        renderer.onMake(view)
        return view
    }

    func updateUIView(_ view: MTKView, context: Context) {
        configure(view: view, using: renderer)
    }

    private func configure(view: MTKView, using renderer: MetalRenderer) {
        view.delegate = renderer
    }
}
