#pragma once

#include "Context/Context.h"
#include <d3d11.h>
#include <d3d11_1.h>
#include <DXGI1_4.h>
#include <wrl.h>
#include <GLFW/glfw3.h>
#include <vector>

using namespace Microsoft::WRL;

class DX11Context : public Context
{
public:
    DX11Context(GLFWwindow* window, int width, int height);
    virtual ComPtr<IUnknown> GetBackBuffer() override;
    virtual void Present() override;
    virtual void DrawIndexed(UINT IndexCount) override;
    virtual void SetViewport(int width, int height) override;
    virtual ComPtr<IUnknown> CreateTexture(uint32_t bind_flag, DXGI_FORMAT format, uint32_t msaa_count, int width, int height, int depth) override;
    virtual ComPtr<IUnknown> CreateBufferSRV(void* data, size_t size, size_t stride) override;
    virtual ComPtr<IUnknown> CreateSamplerAnisotropic() override;
    virtual ComPtr<IUnknown> CreateSamplerShadow() override;

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> device_context;
    ComPtr<ID3DUserDefinedAnnotation> perf;
private:
    virtual void ResizeBackBuffer(int width, int height) override;
    ComPtr<IDXGISwapChain3> swap_chain;
};
