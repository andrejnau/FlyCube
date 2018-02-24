#pragma once

#include <d3d11.h>
#include <d3d11_1.h>
#include <DXGI1_4.h>
#include <wrl.h>
#include <GLFW/glfw3.h>
#include <vector>

using namespace Microsoft::WRL;

class Context
{
public:
    Context(GLFWwindow* window, int width, int height);
    ComPtr<ID3D11Resource> GetBackBuffer();
    void ResizeBackBuffer(int width, int height);
    void Present();
    void DrawIndexed(UINT IndexCount);
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> device_context;
    ComPtr<ID3DUserDefinedAnnotation> perf;
    GLFWwindow* window;
private:
    std::vector<ComPtr<ID3D11RenderTargetView>> rtv;
    ComPtr<ID3D11DepthStencilView> dsv;
    ComPtr<IDXGISwapChain3> swap_chain;
};
