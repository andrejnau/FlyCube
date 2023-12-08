import SwiftUI
import MetalKit

#if os(iOS) || os(tvOS)
typealias ViewRepresentable = UIViewRepresentable
#elseif os(macOS)
typealias ViewRepresentable = NSViewRepresentable
#endif

struct MetalView: ViewRepresentable {
    var renderer: MetalRenderer

    func makeView(context: Context) -> MTKView {
        let view = MTKView(frame: .zero, device: nil)
        configure(view: view, using: renderer)
        return view
    }

    func updateView(_ view: MTKView, context: Context) {
        configure(view: view, using: renderer)
    }

    private func configure(view: MTKView, using renderer: MetalRenderer) {
        view.delegate = renderer
    }
}

#if os(iOS) || os(tvOS)
extension MetalView {
    func makeUIView(context: Context) -> MTKView {
        makeView(context: context)
    }

    func updateUIView(_ uiView: MTKView, context: Context) {
        updateView(uiView, context: context)
    }
}
#elseif os(macOS)
extension MetalView {
    func makeNSView(context: Context) -> MTKView {
        makeView(context: context)
    }

    func updateNSView(_ nsView: MTKView, context: Context) {
        updateView(nsView, context: context)
    }
}
#endif
